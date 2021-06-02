;	This file contains some functions necessary for the kernel, but cannot be done
;	in C.
;

section .text
global loadPML4T
global loadGDT
global loadTSS
global get_cr3

loadPML4T:
	mov cr3, rdi
	ret

get_cr3:
	mov rax, cr3
	ret

loadGDT:
	; This is just a one-time function that loads the gdt.
	; The GDT never ever changes, so no need for a function to load a different
	; one.
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


loadTSS:
	; The TSS however CAN change, and when SMP support is added we'll need
	; a different one for each processor.
	;
	; This function copies a GDT entry to where the TSS should be.
	; rdi should hold a pointer to the gdt
	; entry.
	push rax
	; This loads the entire entry, since the TSS entry is 16 bytes.
	mov rax, [rdi]
	mov [GDT64.tss], rax
	mov rax, [rdi + 8]
	mov [GDT64.tss + 8], rax
	mov ax, 0x28
	ltr ax
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
	.code: equ $ - GDT64 ; KERNEL CODE              0x8
		dw 0  ; limit 0-15
		dw 0  ; base 0-15
		db 0  ; base 16-23
		db 10011010b   ; access
		db 00100000b   ; limit 16-19 | (flags << 4)
		db 0  ; base 24-31
	.data: equ $ - GDT64 ; KERNEL DATA              0x10
		dw 0
		dw 0
		db 0
		db 10010010b
		db 0
		db 0
	.user_code: equ $ - GDT64 ; USER CODE           0x18
		dw 0
		dw 0
		db 0
		db 11111010b  ; access. NOTE: The DC bit might be the culprit for a bug later on.
		db 00100000b
		db 0
	.user_data: equ $ - GDT64 ; USER DATA           0x20
		dw 0
		dw 0
		db 0
		db 11110010b  ; access
		db 00000000b
		db 0
	.tss:  ; Task State Segment                     0x28
		dq 0  ; allocate space for  16 bytes, the size of a long mode TSS.
		dq 0
	.descriptor:
		dw $ - GDT64 - 1
		dq GDT64



