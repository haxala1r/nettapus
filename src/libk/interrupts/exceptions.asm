%include "/home/ezman/Desktop/Hobby_things/Programming/Projects/OS/nettapus/src/libk/interrupts/macros.asm"

SECTION .text
GLOBAL exception_divide_by_zero
GLOBAL exception_double_fault

EXTERN fx_area	; Area we save the SSE registers.

; C functions to handle the exceptions.
EXTERN divide_by_zero_handler
EXTERN kpanic


; The actual interrupt handlers for the exceptions.
exception_divide_by_zero:
	pushfq
	fxsave [fx_area]
	
	PUSHAQ
	
	cld
	call divide_by_zero_handler
	
	
	POPAQ
	
	fxrstor [fx_area]
	popfq
	hlt		; Not much we can do yet...


exception_double_fault:
	mov rsp, temp_stack_top
	jmp kpanic
	hlt
	
SECTION .bss
temp_stack:
	resb 0x1000
temp_stack_top:




