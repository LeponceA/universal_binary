# Universal Binary Program

## Introduction

This code has been realised as a proof of concept for the universal binary format detailed in the master thesis “Universal Binary to Orchestrate Compatibility between Modern Operating Systems”. This code propose a universal binary format that is as general as possible in terms of the systems it can represent.

## Installation

This code has been written in C99. It is meant to be compiled using the [GNU/GCC](https://gcc.gnu.org/) compiler together with the `make` program. To compile the code, go to the main directory of this repository and enter the following command into the terminal:

```bash
make
```

This will generate the `compat_check` program and the `unibin` program. You can run the `compat_check` program to check if your implementation of C respect the basic assumptions made about the types' size. For compatible implementations, the output should be:

```bash
Checking char are 8 bits.
Checking uint8_t are 8 bits.
Checking uint16_t are 16 bits.
Checking uint32_t are 32 bits.
Checking uint64_t are 64 bits.
Checking file offsets are 64 bits.
No incompatibility issues have been encountered.
```

If any of those tests fail, you will need to adapt the source code in order to write the correct amount of data for file operations and be sure that the types are large enough to store values of the sizes above.

## Test

If you got the last output in the Installation section, you can test the universal binary program by simply starting the `unibin` program.

By default, the code generates a universal binary with 2 versions of the application inside the [test/import/](./test/import/) directory. The first version is for Windows NT system using Intel 64 processors (exporting [hello-world.exe](./test/import/hello-world.exe)) and the second is for the current system (exporting [hello-world](./test/import/hello-world), a program compile on Unbuntu 20.04.4 LTS).

The `hello-world` programs are from the [hello-world.c](./test/import/resources/hello-world.c) file if you want to recompile it.

Notice that the only OS supported are Windows NT and some POSIX systems. For the ISA, only x86-64 processors are supported. If your system is not one of them, the program will not work. Future implementation should take into account the extra information field of the OS as well as the ISA extensions.

