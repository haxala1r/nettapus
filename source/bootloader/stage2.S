
.code16
.section .text
.globl _stage2
.type _stage2, @function

_stage2:
	mov $0, %eax
	mov $0, %ecx
	mov $0, %ebx
	
	
	
	popw %dx		//value of initial_dl
	popw %bx  		//address of secondentry
	popw %cx		//address of DPAS
	
	mov %cx, dpas
	mov %bx, partition_entry
	mov %dl, (drive_num)
	
	//change the DPAS entry to read the first sector of second partition.
	movw $rootdir, 4(%ecx)
	movl 8(%ebx), %eax
	movl %eax, 8(%ecx)
	movw $0x01, 2(%ecx) //change amount of sectors to read.
	
	mov %cx, %si
	mov $0x42, %ah
	int $0x13
	
	cmp $0, %ah
	jne halt //puts 'E' and halts if read wasn't successful
	
	
	
	
	//now we check whether it actually is ustar or not. if it is not, halt.
	/*mov $rootdir, %ebx
	movb 257(%ebx), %al
	cmp $0x75, %al
	jne halt*/
	
	
	
	
	//At first I wanted to locate the kernel from real mode,
	//but that appears to be really hard, so we'll do it in C
	//when we enter protected mode.
	//for now we'll just gather as much info as possible,
	//then enter protected mode, locate the kernel and pass execution to it.
	
	//now then, let's get the memory map so that the kernel knows
	//how much memory it has.
	mov $memory_map, %di
	xor %ebx, %ebx
	mov $0x534D4150, %edx
	
	call mem_loop
	
	cmp $memory_map, %di
	je halt //if this jumps, we didn't get any entries. 
	//this is because the mem_loop function sets %di to $mem_loop, and increases as it gets entries.
	//if %di == $mem_loop, then %di was never incremented. thus no entries.
	
	
	//at this point we should have a usable memory map, now we can
	//say goodbye to real mode and enter the weird world of protected mode.
	
	//first enable the A20 line.
	call handle_A20
	//if it returns, then A20 should be active. if it doesn't, then you're in hell.
	
	
	//load a GDT, set the PM bit in %cr0, then long jump into the protected mode part.
	cld
	cli
	lgdt (gdt_descriptor)
	mov %cr0, %eax
	orl $1, %eax
	mov %eax, %cr0
	ljmp $0x8, $pmode_main
	
	
	//if it reaches here, then what the fuck?
	nop

	mov $0x0e, %ah
	mov $0x43, %al
	int $0x10
	
	hlt
//constants, to make life easier.
.equ CODE_SEG, 0x8
.equ DATA_SEG, 0x10
	
halt:
	xor %eax, %eax
	mov $0x0e, %ah
	mov $0x45, %al
	int $0x10
	cli
	hlt
	jmp halt

return:
	ret
	
	

	
	
//info gathering.
mem_loop:
	mov $0xE820, %eax
	mov 24, %ecx
	
	int $0x15
	jc halt
	
	mov $0x534D4150, %edx //some BIOSs apparently trash this.
	cmp %edx, %eax
	jne halt
	
	cmp $0, %ebx
	je return
	add $0x24, %di
	
	jmp mem_loop

//Protected mode shenanigans.
gdt_start:  //a really minimal Global Descriptor Table. the kernel should load its own GDT.
gdt_null://the null entry
	.long 0
	.long 0
gdt_code:	//in this very minimal bootloader, both the code segment and the data segment both start from zero, and span out the entire memory.
	.word 0xffff 	//limit 0:15
	.word 0x0000 	//base 0:15
	.byte 0 		//base 16:23
	.byte 0b10011010	//Access byte
	.byte 0b11001111	//flags and Limit 16:19 4 bit each from left to right.
	.byte 0			//base 24:31
gdt_data:
	.word 0xffff
	.word 0x0000
	.byte 0
	.byte 0b10010010
	.byte 0b11001111
	.byte 0
gdt_end:
gdt_descriptor:
	.word gdt_end - gdt_start - 1
	.long gdt_start



//A20 Line activation
timeout_loop:	//simply a loop in a loop, making it waste some time. I used this to wait for Fast A20 activation thingy.
	add $1, %dx
	cmp $0xFFFF, %dx
	jne timeout_loop
	mov $0, %dx
	add $1, %cx
	cmp 0x40, %cx
	jne timeout_loop
	ret

//checks A20 line, and activates it if it isn't already active.
//halts if it cannot activate it.
handle_A20:
	call check_a20
	
	cmp $1, %al
	je return		//if active, simply skip the other checks.
	
	//check BIOS
	call check_A20_BIOS
	cmp $1, %ax
	je return		//if active, skip other checks.
	
	//check other methods. namely Fast A20 gate.
	in $0x92, %al
	or $2, %al
	out %al, $0x92
	
	//apparently Fast A20 gate can be slow, so we wait for it to activate.
	//(the irony, huh?)
	mov $0, %dx
	mov $0, %cx
	call timeout_loop	
	
	
	
	call check_a20
	cmp $1, %ax
	je return 	//if active, we return.
	
	//if none of those worked, then we give up and halt. and also get depressed because
	//WE'RE FUCKED.
	jmp halt





check_a20:
	pushf
	push %ds
	push %es
	push %di
	push %si
	
	cli
	
	xor %ax, %ax
	mov %ax, %es
	
	mov $0xFFFF, %ax
	mov %ax, %ds
	
	mov $0x500, %di
	mov $0x510, %si
	
	movb %es:(%di), %al
	push %ax
	
	mov %ds:(%si), %al
	push %ax
	
	movb $0, %es:(%di)
	movb $0xFF, %ds:(%si)
	
	cmpb $0xFF, %es:(%di)
	
	pop %ax
	movb %al, %ds:(%si)
	
	pop %ax
	movb %al, %es:(%di)
	
	mov $0, %ax
	je check_a20_exit
	
	mov $1, %ax
	jmp check_a20_exit

check_a20_exit:
	pop %si
	pop %di
	pop %es
	pop %ds
	popf
	ret




check_A20_BIOS:
	mov $0x2403, %ax      	// A20-Gate Support
	int $0x15
	jb a20_fail				// Not supported.
	cmp $0, %ah
	jnz a20_fail			// not supported
	
	mov $0x0e, %ah
	mov $0x47, %al
	int $0x10
	
	
	
	
	mov $0x2402, %ax		// A20-Gate Status.
	int $0x15
	jb a20_fail				//can't get status.
	cmp $0, %ah			
	jnz a20_fail			//can't get status.
	
	cmp $1, %al
	jz a20_already_active			//already active
	
	mov $0x2401, %ax		//activate
	int $0x15
	jb a20_fail
	cmp $0, %ah
	jnz a20_fail
	
	jmp a20_activated

a20_activated:
	xor %ax, %ax
	mov $1, %ax //return 1 if successful
	ret
	
a20_already_active:
	xor %ax, %ax
	mov $1, %ax
	ret

a20_fail:
	//we gotta activate it through some other means.
	xor %ax, %ax //return 0 if not active
	ret

	





partition_entry:
	.word 0
dpas:
	.word 0

drive_num:
	.byte 0



//memory map. we get it with the BIOS call. it can keep 10 entries, at most. probably.
memory_map:
	.skip 24
	.skip 24
	.skip 24
	.skip 24
	.skip 24
	.skip 24
	.skip 24
	.skip 24
	.skip 24
	.skip 24

.code32
pmode_main: //it should jump here after entering protected mode, where we set the segment registers and enter the C part of the bootloader.
	sti
	
	mov $0x10, %eax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	
	mov %ax, %gs
	mov %ax, %ss
	
	//pass a pointer to the archive, cuz we need to.
	mov $rootdir, %eax
	push %eax
	
	call loader_main //this should be linked to this binary.
	cli
	hlt




//this should be at the end, because we're going to be loading the entire archive
//into memory here.
rootdir:
	.skip 512
