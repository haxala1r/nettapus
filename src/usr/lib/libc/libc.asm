GLOBAL _start:function

GLOBAL fork:function
GLOBAL exec:function
GLOBAL wait_syscall:function

GLOBAL open:function
GLOBAL read:function
GLOBAL write:function
GLOBAL close:function
GLOBAL pipe:function

GLOBAL getarg:function
GLOBAL chdir:function

GLOBAL exit:function
EXTERN main


; These are the functions that do raw syscalls.
exit:
	mov rax, 0
	mov rbx, rdi
	int 0x80 ; Does not return.

fork:
	mov rax, 1
	int 0x80
	ret

exec:
	push rbx
	mov rax, 2
	mov rbx, rdi
	mov rcx, rsi
	int 0x80
	pop rbx
	ret ; A successful exec doesn't return, but an error might be returned anyway.

wait_syscall:
	push rbx
	mov rax, 3
	mov rbx, rdi
	int 0x80
	pop rbx
	ret

open:
	push rbx ; This is the only register we need to preserve
	mov rax, 4
	mov rbx, rdi
	mov rcx, rsi
	mov rdx, rdx
	int 0x80
	pop rbx
	ret

read:
	push rbx
	mov rax, 5
	mov rbx, rdi
	mov rcx, rsi
	mov rdx, rdx
	int 0x80
	pop rbx
	ret

write:
	push rbx
	mov rax, 6
	mov rbx, rdi
	mov rcx, rsi
	mov rdx, rdx
	int 0x80
	pop rbx
	ret

close:
	push rbx
	mov rax, 7
	mov rbx, rdi
	int 0x80
	pop rbx
	ret

pipe:
	push rbx
	mov rax, 8
	mov rbx, rdi
	int 0x80
	pop rbx
	ret

chdir:
	push rbx
	mov rax, 9
	mov rbx, rdi
	int 0x80
	pop rbx
	ret

getarg:
	push rbx
	mov rax, 10
	mov rbx, rdi
	mov rcx, rsi
	;mov rdx, rdx
	int 0x80
	pop rbx
	ret

; This is the entry point of a user program.
_start:
	; The kernel passes argc in rax.
	; The individual arguments are accessed with a syscall.
	mov rdi, rax
	call main
	; If main returns, call exit.
	jmp exit
