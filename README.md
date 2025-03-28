# System File Size CLI Tool

This repo contains source code for using a CLI tool to scan one's PC to determine a drive's files sizes and return the largest files in order, along with their path.

**Note:** This tool has only been tested on Windows 11 PC but one could use it for other operating systems.

Source code can be compiled with either gcc or clang compiler for c++17.

## src Directory

This directory contains the source code to scan ones C Drive. There are two files here:
1. single.cpp
2. multi.cpp

The `single.cpp` file is code for a single threaded application. Where as the `multi.cpp` file is for asynchronously scanning one's PC.

## How to use this Code

Compile the code using either gcc or clang (from src directory):

### Single Threaded App

```bash
g++ -o single.exe single.cpp
```

and then run the compiled program.

```bash
.\single.exe
```

### Multi-Threaded App

```bash
g++ -o multi.exe multi.cpp
```

and then run the compiled program.

```bash
.\multi.exe
```

### Run time parameters

use `-depth` or `-num` when running the program to override the default run time parameters.

**Default Parameters:**
- `depth` is used to determine how many directories to recursively scan, starting with the C drive.
  - default this code will scan the C directory with a `depth` of 1 (i.e. only the root directory).
- `num` determines the number of files outputted to the console.
  - default top 10 largest files scanned.

**Examples:**

The following console command will scan the C drive for a depth of 10 (scan no further than 10 files deep into the C drive) and output the top 20 largest files found.

```bash
.\multi.exe -depth 10 -num 20
```

## Code Explanation

This repo only uses the standard library for all containers, algorithms, and filesystem functionality.
