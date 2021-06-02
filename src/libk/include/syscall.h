#ifndef SYSCALL_H
#define SYSCALL_H 1
#include <stdint.h>
#include <stddef.h>
uint64_t syscall_handler(uint64_t rax, uint64_t rbx);


#endif
