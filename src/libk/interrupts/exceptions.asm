%include "src/libk/interrupts/macros.asm"

SECTION .text
GLOBAL exception_divide_by_zero
GLOBAL exception_double_fault

EXTERN fx_area	; Area we save the SSE registers.

; C functions to handle the exceptions.
EXTERN divide_by_zero_handler
EXTERN kpanic


; The actual interrupt handlers for the exceptions.
exception_divide_by_zero:
exception_double_fault:
	mov rsp, fault_stack_top
	mov rbp, rsp
	jmp kpanic

SECTION .bss
fault_stack:
	resb 0x1000
fault_stack_top:
