%include "src/libk/arch/x86-64/interrupts/macros.asm"

SECTION .text
GLOBAL fx_area
GLOBAL loadIDT
GLOBAL irq0
GLOBAL irq1

EXTERN irq0_handler
EXTERN irq1_handler

EXTERN get_current_task



loadIDT:
	pushfq
	cli

	lidt [rdi]

	popfq
	ret



irq0:
	; At this point, the old rsp of the interrupt could get wiped, and we don't care.
	PUSHAQ

	call irq0_handler

	POPAQ
	iretq


irq1:
	pushfq
	cld
	fxsave [fx_area]

	PUSHAQ

	call irq1_handler

	POPAQ

	fxrstor [fx_area]
	popfq
	iretq




SECTION .bss

align 16
fx_area:
	resb 0x200




