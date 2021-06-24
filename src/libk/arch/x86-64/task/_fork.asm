
GLOBAL fork_ret

; This function copies the first task's stack onto the other's, then sets up the
; rax variable in each task, and returns.
; RDI should hold the parent, RSI should hold the child.
; RDX should hold the address to which the child's kernel stack is mapped.
; It is assumed that the current task is the parent and is in RDI.
; It is assumed that only the kernel stack has to be copied.
fork_ret:
	mov QWORD [rdi + 0x80], fork_ret.ret
	mov QWORD [rsi + 0x80], fork_ret.ret

	; These registers have to be preserved in order to be able to return to
	; the C function. The rest of the registers are either in the stack of the
	; caller (presumably a syscall), or are simply not required to be preserved
	; for the caller (the caller is a C function. )
	pushfq
	pop rax
	mov [rdi + 0x90], rax
	mov [rsi + 0x90], rax

	mov [rdi + 8], rbx
	mov [rsi + 8], rbx

	mov [rdi + 0x70], rsp
	mov [rsi + 0x70], rsp

	mov [rdi + 0x78], rbp
	mov [rsi + 0x78], rbp

	mov [rdi + 0x50], r12
	mov [rsi + 0x50], r12

	mov [rdi + 0x58], r13
	mov [rsi + 0x58], r13

	mov [rdi + 0x60], r14
	mov [rsi + 0x60], r14

	mov [rdi + 0x68], r15
	mov [rsi + 0x68], r15

	mov QWORD [rdi + 0x98], cs
	mov QWORD [rsi + 0x98], cs

	mov QWORD [rdi + 0xA0], ss
	mov QWORD [rsi + 0xA0], ss

	mov rcx, 0 ; rcx is a scratch register, and the interrupt handler will restore it.
	mov rax, [rdi + 0xA8]
	sub rax, 0x1000

	mov QWORD [rdi], 1
	mov QWORD [rsi], 0

	.loop:
	mov rdi, [rax + rcx]
	mov [rdx + rcx], rdi

	add rcx, 8

	cmp rcx, 0x1000
	jne fork_ret.loop

	mov rax, 1
	.ret:
	ret
