#!/bin/sh
x86_64-elf-gcc -ffreestanding -c "libc/libc.c" -o libcc.o
nasm -f elf64 "libc/libc.asm" -o libc.o
ar rcs libc.a libcc.o libc.o
rm libcc.o
rm libc.o
OUTLIB="${OUTLIB} libc.a"
