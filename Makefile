

ZLIB=lib/zlib/libz.a
LIBS=$(ZLIB)
INCLUDE=lib

CC=gcc
DBG=-g -fsanitize=address -fsanitize=leak -Wformat=2 -Wduplicated-cond -Wfloat-equal -Wshadow -Wconversion -Wjump-misses-init -Wlogical-not-parentheses -Wnull-dereference
CFLAGS=-Wall -Wextra -pedantic -std=c99 -O2
SRCDIR=./

PATH_WIN32=platform-specific/win32
PATH_POSIX=platform-specific/posix
PATH_X86=platform-specific/x86
NAME=unibin
OBJ=main.o file_header.o compatibility.o files_adder.o unibin.o compat_test.o packages_table.o platform.o $(PATH_WIN32)/win32.o $(PATH_POSIX)/posix.o $(PATH_X86)/x86_64.o

all: unibin check

unibin: $(OBJ)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJ) -L$(LIBS)

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

file_header.o: file_header.c
	$(CC) $(CFLAGS) -c file_header.c -o file_header.o

files_adder.o: files_adder.c
	$(CC) $(CFLAGS) -c files_adder.c -o files_adder.o

compatibility.o: compatibility.c
	$(CC) $(CFLAGS) -c compatibility.c -o compatibility.o

packages_table.o: packages_table.c
	$(CC) $(CFLAGS) -c packages_table.c -o packages_table.o

platform.o: platform.c
	$(CC) $(CFLAGS) -c platform.c -o platform.o

unibin.o: unibin.c
	$(CC) $(CFLAGS) -c unibin.c -o unibin.o

compat_test.o: compat_test.c
	$(CC) $(CFLAGS) -c compat_test.c -o compat_test.o

$(PATH_WIN32)/win32.o: $(PATH_WIN32)/*.c
	$(CC) $(CFLAGS) -c $(PATH_WIN32)/win32compatibility.c -o $(PATH_WIN32)/win32compatibility.o
	$(CC) $(CFLAGS) -c $(PATH_WIN32)/win32filemanip.c -o $(PATH_WIN32)/win32filemanip.o
	ld -r -o $(PATH_WIN32)/win32.o $(PATH_WIN32)/win32compatibility.o $(PATH_WIN32)/win32filemanip.o


$(PATH_POSIX)/posix.o: $(PATH_POSIX)/*.c
	$(CC) $(CFLAGS) -c $(PATH_POSIX)/posixcompatibility.c -o $(PATH_POSIX)/posixcompatibility.o
	$(CC) $(CFLAGS) -c $(PATH_POSIX)/posixfilemanip.c -o $(PATH_POSIX)/posixfilemanip.o
	ld -r -o $(PATH_POSIX)/posix.o $(PATH_POSIX)/posixcompatibility.o $(PATH_POSIX)/posixfilemanip.o

$(PATH_X86)/x86_64.o: $(PATH_X86)/*.c
	$(CC) $(CFLAGS) -c $(PATH_X86)/*.c -o $(PATH_X86)/x86_64.o

check: compatibility_check.c
	$(CC) $(CFLAGS) compatibility_check.c -o compat_check

clean:
	rm $(OBJ)

clean-export:
	rm -r test/export/*
