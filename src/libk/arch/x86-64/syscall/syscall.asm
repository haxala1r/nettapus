%include "src/libk/arch/x86-64/interrupts/macros.asm"
GLOBAL syscall_interrupt
EXTERN syscall_handler

syscall_interrupt:
	cli
	hlt
	mov [rsp-8], rax
	mov ax, 0x10
	mov ds, ax
	mov ss, ax
	mov rax, [rsp-8]
	PUSHAQ
	cld
	mov rdi, rax
	mov rsi, rbx
	call syscall_handler
	mov bx, 0x23
	mov ds, bx
	POPAQ
	iretq
