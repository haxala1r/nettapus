%include "src/libk/arch/x86-64/interrupts/macros.asm"
GLOBAL syscall_interrupt
EXTERN syscall_table
EXTERN syscall_count

; This interrupt MUST NOT be called by a kernel task. It will trash the ds and ss
; with user values otherwise.
syscall_interrupt:
	PUSHAQ
	cld

	cmp [syscall_count], rax
	jb .done ; This was the fix. run it and see.
	je .done
	; Pass the parameters in registers, and call the appropriate syscall handler.
	; The specific system call is determined by rax.
	mov rax, [syscall_table + rax * 0x8]
	mov rdi, rbx
	mov rsi, rcx
	mov rdx, rdx ;

	call rax

	mov [rsp + 14 * 8], rax

	.done:
	POPAQ
	iretq
