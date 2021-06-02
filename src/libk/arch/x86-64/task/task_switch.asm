
%include "src/libk/arch/x86-64/interrupts/macros.asm"

GLOBAL switch_task
GLOBAL task_loader
EXTERN unlock_scheduler
EXTERN get_current_task

; TODO: REWRITE THIS ENTIRE MECHANISM, SO THAT USER-TASKS ARENT ANY DIFFERENT THAN
; KERNEL TASKS. THE TASK SWITCH CODE SHOULD SIMPLY SET REGISTERS & JUMP.
; A GOOD IDEA WOULD BE TO ALWAYS USE IRETQ, REGARDLESS OF RING. THAT WAY,
; YOU DONT NEED TO DO ANYTHING MORE THAN SET VARIABLES (MAYBE HAVE A LOADER FUNC ETC).
; HAVE THE SEGMENT REGISTERS BE PART OF task_registers AND REMOVE THE ring VARIABLE
; FROM task (Or maybe not, but it won't be used by switch_task).
; TODO incorporate this into the actual fucking scheduler.
switch_task:
	; This function does the same thing as above, excpet it returns to userspace
	; instead.
	; RSI should contain a pointer to the registers of the task to be switched to.
	; RDI should contain a pointer to the registers of the task to be switched from.

	cmp rsi, 0
	jz switch_task.reg_return
	cmp rdi, 0
	jz switch_task.load
	; Save the current values for all the registers we're going to push for iretq.
	.save:
	mov QWORD [rdi + 0xA0], ss  ; SS
	mov QWORD [rdi + 0x70], rsp ; RSP
	; We can save rflags later.
	mov QWORD [rdi + 0x98], cs  ; CS
	; Thanks to us saving the ret instruction as RIP, when another task switches
	; to this one, everything will be as if this function returned normally.
	mov QWORD [rdi + 0x80], switch_task.reg_return ; RIP

	; Save rflags. Target flags will be loaded by iretq
	push rax
	pushfq
	pop rax
	mov [rdi + 0x90], rax
	; Don't restore RAX yet!

	; save old CR3 and load new CR3.
	mov rax, cr3
	mov [rdi + 0x88], rax
	pop rax

	; TODO: handle SSE registers as well (fxsave fxrstor)

	; Save the current GPRs.
	mov [rdi], rax
	mov [rdi + 0x08], rbx
	mov [rdi + 0x10], rcx
	mov [rdi + 0x18], rdx
	mov [rdi + 0x20], rdi	; Remember, this doesn't affect rdi.
	mov [rdi + 0x28], rsi
	mov [rdi + 0x30], r8
	mov [rdi + 0x38], r9
	mov [rdi + 0x40], r10
	mov [rdi + 0x48], r11
	mov [rdi + 0x50], r12
	mov [rdi + 0x58], r13
	mov [rdi + 0x60], r14
	mov [rdi + 0x68], r15

	;mov [rdi + 0x70], rsp ; we already saved rsp above.
	mov [rdi + 0x78], rbp ; rbp wasn't changed in this function.

	; Load.
	.load:
	; Now set up the stack how iretq expects to find it.
	push QWORD [rsi + 0xA0]  ; SS
	push QWORD [rsi + 0x70]  ; RSP
	push QWORD [rsi + 0x90]  ; RFLAGS
	push QWORD [rsi + 0x98]  ; CS
	push QWORD [rsi + 0x80]  ; RIP

	mov rax, [rsi + 0x88]
	mov cr3, rax


	; Load new GPRs.
	mov rax, [rsi]
	mov rbx, [rsi + 0x8]
	mov rcx, [rsi + 0x10]
	mov rdx, [rsi + 0x18]
	mov rdi, [rsi + 0x20]	; We don't need this pointer anymore.
		; loading rsi would cause problems, so we'll do that last.
	mov r8, [rsi + 0x30]
	mov r9, [rsi + 0x38]
	mov r10, [rsi + 0x40]
	mov r11, [rsi + 0x48]
	mov r12, [rsi + 0x50]
	mov r13, [rsi + 0x58]
	mov r14, [rsi + 0x60]
	mov r15, [rsi + 0x68]

	; We can't load RSP here, instead, iretq will load it for us.

	mov rbp, [rsi + 0x78]

	; If ds is a user data segment, we cannot load rsi normally after setting ds.
	; We accommodate for this by pushing rsi's future value, then popping it
	; when we're done. (ss is set by iretq.)
	push QWORD [rsi + 0x28]
	push rax
	mov rax, [rsi + 0xA0]
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	pop rax
	pop rsi

	.iret:
	iretq

	.reg_return:
	ret

; This function takes a pointer to a task struct and loads it. The rip should be
; pushed to the stack.
; The main purpose of this function is to unlock the scheduler before delivering
; control to the task. We cannot do this without a loader task.
task_loader:
	call unlock_scheduler
	call get_current_task
	mov rdi, 0
	mov rsi, rax

	pop rax
	mov [rsi + 0x80], rax; Move the real RIP.

	; Now load the actual segment registers based on ring.
	mov rax, [rsi + 0xC8] ; ring
	cmp rax, 0
	jz task_loader.ring0
	jmp task_loader.ring3

	.ring3:
	mov QWORD [rsi + 0xA0], 0x20 | 3
	mov QWORD [rsi + 0x98], 0x18 | 3
	jmp task_loader.ring_done

	.ring0:
	mov QWORD [rsi + 0xA0], 0x10
	mov QWORD [rsi + 0x98], 0x08

	.ring_done:
	call switch_task
