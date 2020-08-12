.code16


.section .text
.globl _start
.type _start, @function

_start:
	//zero out some stuff
	mov $0, %ax
	
	mov %ax, %ds
	mov %ax, %es
	//mov $0x10, %ax
	mov %ax, %fs
	mov %ax, %gs
	//temporary stack.
	//should and will be changed when entering protected mode.
	xor %ax, %ax
	mov $0x7c00, %bp
	mov %ax, %ss
	mov %bp, %sp
	
	
	
	mov %dl, initial_dl
	
	
	
	
	//now we can do stuff.	
	//first we read the stage2 code in the first partition.
	//move the starting sector number into the DPAS entry.
	mov (firstentry+0x8), %ax
	mov %ax, (dpas+0x8)
	
	xor %ax, %ax
	
	//then load something from there into memory.
	mov $0x42, %ah
	mov initial_dl, %dl
	mov $dpas, %si
	int $0x13
	
	
	
	jc _
	//push some values before jumping, so that
	//stage2 can access them.
	cmp $0, %ah
	jne _
	
	
	
	
	pushw $dpas //we can just reuse this DPAS entry to read stuff.
	pushw $secondentry 	//so that we can find sysroot.
	pushw initial_dl
	
	
	jmp __stage2
	//if it reaches here, then an error occured.
	//try to make the error code kinda readable by turning it into ascii,
	//I know this isn't even an acceptable way of doing this but I ran out of ideas okay?
_:	mov %ah, %al
	mov $0x0e, %ah
	add $0x35, %al
	int $0x10
	
	
	cli
	hlt

//Disk Packet Address Structure.
dpas:
	.byte 16
	.byte 0
	
	//num. of sectors to transfer. I just put 8 to guarantee all of stage2 is loaded
	.word 8 
	
	//address to load to.
	.long __stage2
	
	//starting LBA first 32 bits
	.long 1
	//starting LBA last 32 bits
	.long 0




initial_dl:
	.byte 0x00

	
//we gotta make this a valid MBR
.org 0x1B8
	.word 0x1501
	.word 0x1501
	.word 0x0000
//partition table
.org 0x1BE //entry 1
firstentry:
	//partition attributes. 0x80=bootable.
	.byte 0x80
	
	.byte 0
	.word 0
	
	//partition type
	.byte 0x81 
	
	.byte 0
	.word 0
	
	.long 2048 //starting sector number
	
	.long 0x00010000 //size(in sectors.)

.org 0x1CE //entry 2, system root. you can change its size.
secondentry: 
	.byte 0x00  //we don't want this one to be bootable.
	
	.byte 0
	.word 0
	
	//partition type. doesn't really matter, I chose "FAT16 < 32M" because fat16
	.byte 0x83
	
	.byte 0
	.word 0
	
	.long 67584 //starting sector number
	//size, in sectors. I wanted something small, so 20 MiB for now.
	.long 40960  
.org 0x1DE //entry 3
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
.org 0x1EE //entry 4
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0

//now the magic word
.org 0x1FE
	.word 0xAA55

//First 512 bytes.
///////////////////////////////////////////////////////
//The rest of the binary. we can now look for the kernel we want to execute,
//without the 512 bytes limitation


__stage2:
	//this shouldn't be loaded into memory, instead the stage2 loader should
	//be loaded here. this is just for test purposes, a placeholder, if you will.
	//stage2 loader should be burned into the first partition.
	mov $0x0e, %ah
	mov $0x40, %al
	int $0x10
	nop
	cli
	hlt

 
