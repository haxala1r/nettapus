%include "src/libk/arch/x86-64/interrupts/macros.asm"

SECTION .text
GLOBAL exception_divide_by_zero
GLOBAL exception_gpf
GLOBAL exception_ud
GLOBAL exception_ts
GLOBAL exception_np
GLOBAL exception_ss
GLOBAL exception_pf
GLOBAL exception_double_fault

EXTERN fx_area	; Area we save the SSE registers.

; C functions to handle the exceptions.
EXTERN divide_by_zero_handler
EXTERN exception_gpf_handler
EXTERN exception_ud_handler
EXTERN exception_ts_handler
EXTERN exception_np_handler
EXTERN exception_ss_handler
EXTERN exception_pf_handler
EXTERN kpanic


; The actual interrupt handlers for the exceptions.
exception_gpf: ; General Protection Fault.0xFFFFFFFF8020A36C
	mov ax, 0x10
	mov ss, ax
	mov ds, ax
	mov rdi, [rsp+8]
	PUSHAQ
	cld
	call exception_gpf_handler
	POPAQ
	cli
	hlt
exception_ud: ; Invalid opcode.
	mov ax, 0x10
	mov ss, ax
	mov ds, ax ; GCC will not use any other segments, so we're fine.
	PUSHAQ
	cld
	call exception_ud_handler
	POPAQ ; So that we can see registers in qemu when we halt.
	cli
	hlt
exception_ts: ; Invalid TSS.
	mov ax, 0x10
	mov ss, ax
	mov ds, ax
	PUSHAQ
	cld
	call exception_ts_handler
	POPAQ
	cli
	hlt
exception_np: ; Segment Not Present
	mov ax, 0x10
	mov ss, ax
	mov ds, ax
	PUSHAQ
	cld
	call exception_np_handler
	POPAQ
	cli
	hlt
exception_ss: ; Stack Fault.
	mov ax, 0x10
	mov ss, ax
	mov ds, ax
	PUSHAQ
	cld
	call exception_ss_handler
	POPAQ
	cli
	hlt
exception_pf: ; Page Fault.
	mov ax, 0x10
	mov ss, ax
	mov ds, ax
	pop rax
	PUSHAQ
	cld
	mov rdi, cr2
	mov rsi, rax
	call exception_pf_handler
	POPAQ
	cli
	hlt

exception_divide_by_zero:
exception_double_fault: ; Double Fault.
	mov ax, 0x10
	mov ss, ax
	mov ds, ax
	mov rsp, fault_stack_top
	mov rbp, rsp
	jmp kpanic

SECTION .bss
fault_stack:
	resb 0x1000
fault_stack_top:
