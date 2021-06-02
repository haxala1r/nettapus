%include "src/libk/arch/x86-64/interrupts/macros.asm"

SECTION .text
GLOBAL fx_area
GLOBAL loadIDT
GLOBAL irq0
GLOBAL irq1

EXTERN irq0_handler
EXTERN irq1_handler

EXTERN get_current_task



loadIDT:
	pushfq
	cli

	lidt [rdi]

	popfq
	ret



irq0:
	; Because the handler isn't guaranteed to return at all, and if it does, it
	; needs the stack to be the way it left, we need to swtich to the current_task's
	; kernel stack.
	cld
	mov [rsp - 0x8], rax
	mov ax, 0x10
	mov ss, ax
	mov rax, [rsp - 0x8]
	push rax
	push rbp

	call get_current_task ; This will only put the current task at rax
	cmp rax, 0
	je irq0.call_handler
	cmp QWORD [rax + 0xA8], 0
	je irq0.call_handler

	.prepare_stack:
	mov rbp, rsp ; the old stack. We will load old rbp back later.
	mov rsp, [rax + 0xA8] ; load the task's kernel stack.

	; copy the info on the previous stack to the new one.
	; determine whether ss & rsp were pushed. TODO: complete this.
	cmp QWORD [rbp + 0x20], 0x18 | 3

	;jne irq0.copy_stack
	push QWORD [rbp + 0x30] ; ss
	push QWORD [rbp + 0x28] ; rsp
	.copy_stack:

	push QWORD [rbp + 0x20] ; rflags
	push QWORD [rbp + 0x18] ; cs
	push QWORD [rbp + 0x10] ; rip
	push QWORD [rbp + 0x08] ; rax
	push QWORD [rbp] ; rbp

	; At this point, we successfully switched to a proper stack.
	.call_handler:
	pop rbp ; restore rbp
	pop rax ; also rax
	PUSHAQ

	call irq0_handler

	POPAQ
	iretq


irq1:
	pushfq
	cld
	fxsave [fx_area]

	PUSHAQ

	call irq1_handler

	POPAQ

	fxrstor [fx_area]
	popfq
	iretq




SECTION .bss

align 16
fx_area:
	resb 0x200




