
%include "/home/ezman/Desktop/Hobby_things/Programming/Projects/OS/nettapus/src/libk/interrupts/macros.asm"

GLOBAL switch_task



switch_task:
	; RSI should contain a pointer to the registers of the task to be switched to.
	; RDI should contain a pointer to the registers of the task to be switched from.
	; The structure is defined in task.h
	
	cmp rdi, 0
	je switch_task.restore
	
	push rax 	; Save rax.
	
	; We need to save RIP. Not really, we can just specify whatever we want.
	mov rax, switch_task.return
	mov [rdi + 0x80], rax	; The new RIP will be loaded later.
	
	; Let's do CR3 now.
	
	mov rax, cr3
	
	mov [rdi + 0x88], rax			; Save current cr3. 
	mov rax, [rsi + 0x88]			; Load new cr3.
	
	mov cr3, rax
	
	; Now RFLAGS.
	pushfq
	pop rax
	
	mov [rdi + 0x90], rax	; Save current RFLAGS.
	mov rax, [rsi + 0x90]		; Load the new RFLAGS.
	
	push rax
	popfq
	
	
	; Now the SSE registers. 
	; We need to make sure the address is 16-byte aligned. 
	push rbx
	
	mov rbx, 0
	mov rax, rdi
	and rax, 0xF
	jz switch_task.fx_save
	
	mov rbx, 0x10
	sub rbx, rax
	
	.fx_save:
	fxsave [rdi + 0xA0 + rbx]	

	
	
	; Restore the registers, because we need to save the general-purpose registers. 
	pop rbx
	pop rax
	
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
	
	mov [rdi + 0x70], rsp
	mov [rdi + 0x78], rbp
	
	
	.restore:
	
	; We need to make sure the new registers are also 16-byte aligned.
	mov rbx, 0
	mov rax, rsi
	and rax, 0xF
	jz switch_task.fx_rstor
	
	mov rbx, 0x10
	sub rbx, rax
	
	.fx_rstor:
	fxrstor [rsi + 0xA0 + rbx]
	
	; Now we load the General-purpose registers  of the new task.
	
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
	
	mov rsp, [rsi + 0x70]
	mov rbp, [rsi + 0x78]
	
	; Now we load the new RIP to the stack.
	
	push rax
	
	mov rax, [rsi + 0x80]
	xchg [rsp], rax			; This puts the newly loaded RIP (in RAX) on the stack, and restores RAX.	
	
	; And finally, we can load our new RSI value
	
	mov rsi, [rsi + 0x28]
	
	; Now we "return", and everything should be fine.
	
	.return:
	ret
	

