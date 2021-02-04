
%include "macros.asm"

SECTION .text
GLOBAL exception_divide_by_zero
GLOBAL exception_double_fault

EXTERN fx_area	; Area we save the SSE registers.

; C functions to handle the exceptions.
EXTERN divide_by_zero_handler
EXTERN double_fault_handler



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
	hlt
	pushfq
	fxsave [fx_area]
	PUSHAQ
	
	cld 
	call double_fault_handler
	
	POPAQ
	fxrstor [fx_area]
	popfq
	hlt
	






