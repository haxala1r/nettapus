;	This file contains some functions necessary for the kernel, but cannot be done
;	in C. 
;
	
section .text
global loadPML4T
global loadGDT

loadPML4T:
	mov cr3, rdi
	ret


loadGDT:
	push rax
	lgdt [GDT64.descriptor]
	
	
	mov rax, GDT64.code
	push rax
	mov rax, $_gdt_loaded
	push rax
	jmp far [rsp]
	
	
	
_gdt_loaded:
	pop rax
	pop rax
	
	mov ax, GDT64.data
	mov ds, ax
	mov ss, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	
	; Let's enable SSE support as well, while we're at it.
	
	; Disable Coprocessor emulation, enable monitoring.
	mov rax, cr0
	and ax, 0xFFFB
	or ax, 0x2
	mov cr0, rax
	
	; Set CR4.OSFXSR and CR4.OSXMMEXCPT.
	mov rax, cr4
	or ax, 3 << 9
	mov cr4, rax
	pop rax
	ret
	
	



GDT64:
	.null: equ $ - GDT64
		dw 0xFFFF
		dw 0
		db 0
		db 0
		db 1
		db 0
	.code: equ $ - GDT64
		dw 0
		dw 0
		db 0
		db 10011010b
		db 10101111b
		db 0
	.data: equ $ - GDT64
		dw 0
		dw 0
		db 0
		db 10010010b
		db 0
		db 0
	.descriptor:
		dw $ - GDT64 - 1
		dq GDT64



