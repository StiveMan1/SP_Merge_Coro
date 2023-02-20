#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
//    int x = 0;
//    if (temp == NULL) {
//        x = 1;
//        temp = malloc(sizeof(int) * (fn - st));
//    }
    size_t mid = (st + fn) / 2;
    merge_sort_split(st, mid, data, temp);
    merge_sort_split(mid, fn, data, temp);
    merge_sort_combine(st, mid, mid, fn, data, temp);
//    if(x) free(temp);
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
int p_l = -1, p_n;
int coro_n = 0;

#define yield_call(cal)                                                             \
gettimeofday(&start, NULL);                                                         \
{cal}                                                                               \
do{                                                                                 \
    gettimeofday(&stop, NULL);                                                      \
}while(p_l > (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec);\
time += (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;      \
coro_yield();


static int coroutine_merge(void *context) {
    int coro_id = ++coro_n;
    struct merge_st *merge_o = merge_new();
    struct coro *this = coro_this();

    struct timeval stop, start;
    unsigned long long time = 0;
    while (files_done < files_n) {
        int file_id = files_done++;
        yield_call({
                       printf("Coro %d file open : %s\n", coro_id, files[file_id]);
                       merge_open(merge_o, files[file_id]);
                   })
        yield_call({
                       merge_sort(merge_o);
                       printf("Coro %d sorted : %s\n", coro_id, files[file_id]);
                   })
        yield_call({
                       merge_concat(context, merge_o);
                       printf("Coro %d concat : %s\n", coro_id, files[file_id]);
                   })
    }
    merge_free(merge_o);
    printf("\nCoro %d switches : %llu\nCoro %d time : %llu us\n", coro_id, coro_switch_count(this), coro_id, time);
    return 0;
}

void get_args(int argc, char **argv) {
    files_n = argc - 1;
    files = argv + 1;

    printf("Number of coro (<= 0 - auto): ");
    char data[256];
    scanf("%s", data);
    p_n = strtol(data, NULL, 10);
    if (p_n <= 0) p_n = files_n;

    printf("Target latency (<= 0 - with out): ");
    scanf("%s", data);
    p_l = strtol(data, NULL, 10);
    if (p_l <= 0) p_l = -1;
    else p_l = p_l * 1000 / p_n;
}

int main(int argc, char **argv) {
    get_args(argc, argv);

    struct timeval stop, start;
    gettimeofday(&start, NULL);

    struct merge_st *result_merge = merge_new();
    coro_sched_init();
    for (int i = 0; i < p_n; ++i) {
        coro_new(coroutine_merge, result_merge);
    }
    struct coro *c;
    while ((c = coro_sched_wait()) != NULL) {
        coro_delete(c);
    }
    merge_save(result_merge, "result.txt");
    gettimeofday(&stop, NULL);
    printf("\nTotal time : %lu us\n", (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec);
    return 0;
}

