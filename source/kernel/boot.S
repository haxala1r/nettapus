/* Declare constants for the multiboot header. */
.set ALIGN,    1<<0             /* align loaded modules on page boundaries */
.set MEMINFO,  1<<1             /* provide memory map */
.set FLAGS,    ALIGN | MEMINFO  /* this is the Multiboot 'flag' field */
.set MAGIC,    0x1BADB002       /* 'magic number' lets bootloader find the header */
.set CHECKSUM, -(MAGIC + FLAGS) /* checksum of above, to prove we are multiboot */
 
/* 
Declare a multiboot header that marks the program as a kernel. These are magic
values that are documented in the multiboot standard. The bootloader will
search for this signature in the first 8 KiB of the kernel file, aligned at a
32-bit boundary. The signature is in its own section so the header can be
forced to be within the first 8 KiB of the kernel file.
*/
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM
 
//this is a small stack, and it's for kernel usage.
.section .bss
.align 16
stack_bottom:  
.skip 16384 # 16 KiB
stack_top:
 

	
	
	
.section .text
.global _start
.type _start, @function
_start:
	/* here the bootloader puts us in 32-bit protected mode,
	and we can do whatever the hell we want here. */
	/*we'll enable paging, set up the kernel in the higher half, then
	call the kernel. a lot to do, if you ask me.*/
	/*Well apparently I suck at this so I gave up on that last part.*/

	


	mov $stack_top, %esp
	
	push %ebx
	
	call kernel_main
 
	
	cli
1:	hlt
	jmp 1b
 
/*
Set the size of the _start symbol to the current location '.' minus its start.
This is useful when debugging or when you implement call tracing.
*/
.size _start, . - _start

/*some functions to do paging. called from the kernel. */
.globl loadPageDirectory
loadPageDirectory:
	push %ebp
	mov %esp, %ebp
	mov 8(%esp), %eax
	mov %eax, %cr3
	mov %ebp, %esp
	pop %ebp
	ret

.globl enablePaging
enablePaging:
	push %ebp
	mov %esp, %ebp
	mov %cr0, %eax
	or $0x80000000, %eax
	mov %eax, %cr0
	mov %ebp, %esp
	pop %ebp
	ret
	






