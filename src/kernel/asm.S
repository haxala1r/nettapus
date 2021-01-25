/*  This file contains some functions necessary for the kernel, but cannot be done
	in C. */
	
.section .text
.globl loadPML4T
.type loadPML4T, @function
loadPML4T:
	mov %rdi, %cr3
	ret



