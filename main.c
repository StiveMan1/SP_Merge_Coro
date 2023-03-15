#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "libcoro.h"
#include <sys/time.h>


struct merge_st {
    int *data;
    size_t size, mx_size;
};

struct merge_st *merge_new() {
    struct merge_st *res = malloc(sizeof(struct merge_st));
    res->data = NULL;
    res->size = res->mx_size = 0;
    return res;
}

void merge_free(struct merge_st *res) {
    if (res == NULL) return;
    if (res->data != NULL) free(res->data);
    free(res);
}

void merge_resize(struct merge_st *res, size_t size) {
    if (res->data == NULL) {
        res->mx_size = size;
        res->data = malloc(sizeof(int) * size);
        for (size_t i = 0; i < size; i++) res->data[i] = 0;
    } else if (res->mx_size < size) {
        res->data = realloc(res->data, sizeof(int) * size);
        for (size_t i = res->mx_size; i < size; i++) res->data[i] = 0;
        res->mx_size = size;
    }
    if (res->size > size) for (size_t i = size; i < res->size; i++) res->data[i] = 0;
    res->size = size;
}

void merge_open(struct merge_st *res, char *file_name) {
    FILE *ptr = fopen(file_name, "r");
    if (ptr == NULL) return merge_resize(res, 0);

    int num;
    size_t size = 0;
    while (fscanf(ptr, "%d", &num) == 1) size++;
    fseek(ptr, 0, SEEK_SET);

    merge_resize(res, size);
    for (size_t i = 0; i < size; i++) fscanf(ptr, "%d", &res->data[i]);
    fclose(ptr);
}

void merge_save(struct merge_st *res, char *file_name) {
    FILE *ptr = fopen(file_name, "w");
    if (ptr != NULL) {
        for (size_t i = 0; i < res->size; i++)fprintf(ptr, "%d ", res->data[i]);
        fclose(ptr);
    }
}

void merge_sort_combine(size_t st1, size_t st2, size_t fn1, size_t fn2, int *data, int *temp) {
    size_t st = st1;
    size_t pos = st1;
    while (st1 < fn1 || st2 < fn2) {
        if (st1 == fn1) {
            temp[pos++] = data[st2++];
        } else if (st2 == fn2) {
            temp[pos++] = data[st1++];
        } else {
            if (data[st1] <= data[st2]) {
                temp[pos++] = data[st1++];
            } else {
                temp[pos++] = data[st2++];
            }
        }
    }
    for (; st < fn2; st++) {
        data[st] = temp[st];
        temp[st] = 0;
    }
}

void merge_sort_split(size_t st, size_t fn, int *data, int *temp) {
    if (st + 1 >= fn) return;
    size_t mid = (st + fn) / 2;
    merge_sort_split(st, mid, data, temp);
    coro_yield();
    merge_sort_split(mid, fn, data, temp);
    coro_yield();
    merge_sort_combine(st, mid, mid, fn, data, temp);
}

void merge_concat(struct merge_st *res, const struct merge_st *obj) {
    size_t size1 = res->size;
    merge_resize(res, res->size + obj->size);
    memcpy(res->data + size1, obj->data, obj->size * sizeof(int));

    int *temp = malloc(sizeof(int) * res->size);
    merge_sort_combine(0, size1, size1, res->size, res->data, temp);
    free(temp);
}

void merge_sort(struct merge_st *res) {
    int *temp = malloc(sizeof(int) * res->size);
    merge_sort_split(0, res->size, res->data, temp);
    free(temp);
}

int files_n, files_done = 0;
char **files;
int p_l, p_n;
int coro_n = 0;



static int coroutine_merge(void *context) {
    int coro_id = ++coro_n;
    struct merge_st *merge_o = merge_new();

    printf("Coro %d started\n", coro_id);
    while (files_done < files_n) {
        int file_id = files_done++;
        printf("Coro %d file open : %s\n", coro_id, files[file_id]);
        merge_open(merge_o, files[file_id]);
        merge_sort(merge_o);
        merge_concat(context, merge_o);
        coro_yield();
    }
    merge_free(merge_o);
    printf("Coro %d finished : \n", coro_id);
    return 0;
}

void get_args(int argc, char **argv) {
    int _n = -1, _l = -1;
    for (int i = 1; i < argc; i++) {
        if (memcmp(argv[i], "-n", 2) == 0) _n = i + 1;
        if (memcmp(argv[i], "-l", 2) == 0) _l = i + 1;
    }
    if (_n >= argc) perror("Number of Coro is not defined");
    if (_n != -1) p_n = strtol(argv[_n], NULL, 10);
    else p_n = argc - 1 - 2 * (_l != -1);

    if (_l >= argc) perror("Target latency is not defined");
    if (_l != -1) {
        p_l = strtol(argv[_l], NULL, 10);
        if (p_l <= 0) p_l = 0;
        else p_l = p_l * 1000 / p_n;
        time_quant = p_l;
    } else {
        time_quant = 0;
    }

    files_n = argc - 1 - 2 * (_l != -1) - 2 * (_n != -1);
    files = malloc(sizeof(char *) * files_n);

    for (int i = 1, j = 0; i < argc; i++) {
        if (memcmp(argv[i], "-n", 2) == 0 || memcmp(argv[i], "-l", 2) == 0) {
            i++;
            continue;
        }
        files[j++] = argv[i];
    }
    printf("-n : %d\n", p_n);
    printf("-l : %d\n", p_l);
    printf("files : ");
    for(int i=0;i<files_n;i++) printf("%s ", files[i]); printf("\n");
}

int main(int argc, char **argv) {
    get_args(argc, argv);

    struct merge_st *result_merge = merge_new();
    coro_sched_init();
    for (int i = 0; i < p_n; ++i) {
        coro_new(coroutine_merge, result_merge);
    }

    struct timespec start, stop;
    clock_gettime (CLOCK_MONOTONIC, &start);

    struct coro *c;
    while ((c = coro_sched_wait()) != NULL) {
        coro_delete(c);
        printf("\tswitches : %llu\n", coro_switch_count(c));
        printf("\ttime : %llu us\n", coro_delta_time(c));
    }

    merge_save(result_merge, "result.txt");
    merge_free(result_merge);
    free(files);
    clock_gettime (CLOCK_MONOTONIC, &stop);
    printf("\nTotal time : %llu us\n", time_to_us(stop) - time_to_us(start));
    return 0;
}