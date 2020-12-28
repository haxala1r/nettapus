/* Declare constants for the multiboot header. */
.set ALIGN,    	1<<0             /* align loaded modules on page boundaries */
.set MEMINFO,  	1<<1             /* provide memory map */
.set VIDEOINFO, 1<<2				/* Provide video mode info, and also do some more stuff. */
.set FLAGS,    	VIDEOINFO | ALIGN | MEMINFO  /* this is the Multiboot 'flag' field */
.set MAGIC,    	0x1BADB002       /* 'magic number' lets bootloader find the header */
.set CHECKSUM, 	-(MAGIC + FLAGS) /* checksum of above, to prove we are multiboot */
 
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
.skip 24	//other fiels are not necessary.
.long 0		//boot us in linear graphics-mode.
.long 0 	//width: no preference.
.long 0 	//height: no preference.
.long 0		//depth: no preference.


 
//this is a small stack, and it's for kernel usage.
.section .text.kernelprep
.align 16
stack_bottom:  
.skip 16384 # 16 KiB
.skip 16384
.skip 16384
stack_top:
 

	
mbd:
.skip 8

	
.section .text.kernelprep
.global _start
.type _start, @function
_start:
	/* here the bootloader puts us in 32-bit protected mode,
	and we can do whatever the hell we want here. */
	/*we'll enable paging, set up the kernel in the higher half, then
	call the kernel. a lot to do, if you ask me.*/
	
	cli
	


	mov $stack_top, %esp
	
	mov %ebx, mbd
	
	/*this will set a temporary page directory and enable paging.
	after this returns, we will want to switch to the proper stack,
	since we were using a temporary address before. then the function after
	this will set up a proper page directory, and then we can call kernel_main
	with the multiboot header as a parameter.*/
	
	call kernel_prep1
	
	
	/* after this returns, we add 0xC0000000 to sp, beacuse this
	is a higher-half kernel, and the new stack pointer
	will be equal to 0xC0000000 + old sp. Hope that is clear enough.*/
	
	mov $stack_top, %eax
	add $0xC0000000, %eax
	mov %eax, %esp
	mov %esp, %ebp
	
	mov mbd, %ebx
	push %ebx
	/* we pass the multiboot header thing (which gives us info about the system)
	to this function. kernel_prep2 is responsible for properly
	setting up the Memory Manager (TM) and making sure the heap and
	basically everything else is working properly. kernel_prep2()
	is not supposed to return. it should instead unmap the part of memory
	(which contains this function as well) and (after setting everything up)
	pass control to kernel_main.*/
	call kernel_prep2
	
	
	cli
1:	hlt
	jmp 1b
 
/*
Set the size of the _start symbol to the current location '.' minus its start.
This is useful when debugging or when you implement call tracing.
*/
.size _start, . - _start

/* some functions to do paging. called from the kernel. */
.section .text

.globl loadPageDirectory
.type loadPageDirectory, @function
loadPageDirectory:

	push %ebp
	mov %esp, %ebp
	
	mov 8(%esp), %eax
	mov %eax, %cr3
	
	mov %ebp, %esp
	pop %ebp
	
	ret

.globl enablePaging
.type enablePaging, @function
enablePaging:
	push %ebp
	mov %esp, %ebp
	
	mov %cr0, %eax
	or $0x80000000, %eax
	mov %eax, %cr0
	
	mov %ebp, %esp
	pop %ebp
	ret
	







