# CS527 Lab 5 - Simple Version

This version is written to be easier to read and study. It keeps the main Lab 5 ideas:

- compiler + computer system in one executable
- 32-bit integer registers x0-x255
- 32-bit elements in vector registers v0-v31
- 8 elements per vector register
- byte-addressable physical memory
- paging and page tables
- four processor IDs
- OS loader and round-robin time slices
- labels and branch instructions
- Print instruction
- legacy Read and Write instructions

## Files

- `main.c` - program entry point
- `compiler.c/h` - converts program.txt to program.byte
- `processor.c/h` - fetch and execute instructions
- `memory.c/h` - physical memory and byte-file handling
- `os.c/h` - loader, page tables and scheduler
- `Makefile` - Linux/WSL/MinGW build
- `BUILD_WINDOWS.bat` - Windows build shortcut
- `tests/` - sample programs

## Linux / WSL

```bash
make clean
make
./cs527_lab5 tests/sum_array.txt tests/data.byte
```

## Windows MinGW

```powershell
mingw32-make clean
mingw32-make
.\cs527_lab5.exe tests\sum_array.txt tests\data.byte
```

The important point is: run `make` from the folder that contains the Makefile, NOT from `tests`.

## Interactive shell

```bash
./cs527_lab5
```

Then type:

```text
tests/sum_array.txt tests/data.byte
```

or on Windows:

```text
tests\sum_array.txt tests\data.byte
```

Type `exit` to stop the shell.

## Important Lab 5 ideas

Each instruction is four bytes:

```text
opcode destination operand1 operand2
```

Instruction memory uses two 512-byte pages. Data memory uses eight 512-byte pages. A task therefore needs 10 logical pages. Physical frame 0 is reserved.

The processor translates a logical data address through the task's page table before accessing physical memory.


## New vector programs

### 1. Vector sum of two arrays

`tests/vector_sum_arrays.txt` implements:

- `N` at data address `0x0`
- Array A at `0x4`
- Array B at `0x4 + 4N`
- Result at `0x4 + 8N`
- 8 integers are processed per vector instruction

Run:

```bash
./cs527_lab5 tests/vector_sum_arrays.txt tests/vector_sum_data.byte
```

Windows:

```powershell
.\cs527_lab5.exe tests\vector_sum_arrays.txt tests\vector_sum_data.byte
```

### 2. Vector FIR filter

`tests/fir_vector.txt` implements the FIR pseudo-code using vector load, vector multiply and a horizontal vector sum.

- `N` at `0x0`
- 8 weights at `0x4` to `0x23`
- Input starts at `0x24`
- Output starts at `0x100`
- `N <= 64` and is a multiple of 8
- `VSum xN = vM` is a new vector reduction instruction

Run:

```bash
./cs527_lab5 tests/fir_vector.txt tests/fir_data.byte
```

Windows:

```powershell
.\cs527_lab5.exe tests\fir_vector.txt tests\fir_data.byte
```

## Features

- Mini computer system simulator
- Compiler and instruction processing
- Memory management
- Scalar and vector registers
- Vector operations
- Test programs for array operations and FIR filtering
