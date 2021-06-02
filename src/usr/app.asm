[BITS 64]
org 0x40000000
_start:
	mov rcx, 0xFFFF
	.loop:
	mov rax, rcx
	int 0x80
	loop _start.loop
	jmp _start
