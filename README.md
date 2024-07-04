# File Sorting and Merging using Coroutine-Based Merge Sort

This project implements a coroutine-based merge sort for sorting integers stored in multiple files. Each file contains
integers in arbitrary order separated by whitespaces. The goal is to sort each file individually using a coroutine-based
approach and then merge the sorted data into a single output file.

## Requirements

* Each file should be sorted independently using coroutines.
* Sorting algorithms must be implemented without using built-in sorting functions like `qsort()` or external sorting
  utilities.
* Sorting complexity for each file should be less than `𝑂(𝑁2)`.
* The total execution time, including sorting and merging, should be optimized to run within a specified limit.
* Files are ASCII-encoded plain text files.

## Implementation Steps

### 1. Sorting Implementation:

* Implement sorting of a single file without coroutines using a chosen sorting algorithm such as quicksort.
* xtend sorting to multiple files and integrate coroutine-based multitasking. Each coroutine will handle the sorting of
  one file.
* Implement coroutines using yield operations to switch between sorting tasks.

### 2. Merge Sort:

* After each file is individually sorted using coroutines, merge the sorted sequences into a single output file. This
  step
  can be done sequentially in the main program.

### 3. Performance Considerations:

* Ensure that the coroutines handle switching efficiently, and measure coroutine work time excluding wait times.
* Optimize sorting algorithms and coroutine scheduling to meet performance targets.

### 4. Command Line Interface:

* Input consists of file names to be sorted, provided as command line arguments.
* Optionally, additional parameters like target latency (in microseconds) and coroutine count can be specified for bonus
  tasks.

## Restrictions

* Use lower-level file handling functions like `open()`, `read()`, `write()`, and `close()` for file operations.
* Avoid using high-level C++ STL containers and functions such as `std::iostream`.
* Ensure all files fit into memory simultaneously.
* Final merge sort operation can be implemented directly i`n main()` without coroutines.

## Usage

```bash
./main file1.txt file2.txt file3.txt file4.txt file5.txt file6.txt
```

## Testing
For testing purposes, files can be generated using a provided Python script (`generator.py`) with specific
characteristics (e.g., number of integers, range of integers).

```bash
python3 generator.py -f test1.txt -c 10000 -m 10000
python3 generator.py -f test2.txt -c 10000 -m 10000
...
```

After sorting, results can be validated using a provided validation script (`checker.py`).

## Conclusion

This project demonstrates the use of coroutines in sorting multiple files concurrently while adhering to performance
constraints and utilizing lower-level file operations for efficiency. It aims to optimize sorting and merging operations
to achieve high performance within specified hardware limits.