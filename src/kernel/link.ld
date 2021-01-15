ENTRY(_start)

SECTIONS {
	
	/* 1M is a conventional place to begin, apparently.*/
	. = 0x00100000;
	
	.mboot BLOCK(4K) : ALIGN(4K) 
	{
		kernel_start = .;
		*(.multiboot)
		*(.text.kernelprep)
	} 
	
	/* because we have a higher-half kernel*/
	. += 0xC0000000;
	
	.text ALIGN(4K) : AT(ADDR (.text) - 0xC0000000)
	{
		*(.text)
	}
	/*read-only data*/
	.rodata ALIGN(4K) : AT(ADDR (.rodata) - 0xC0000000)
	{
		*(.rodata)
	}
	/*initialised data*/
	.data ALIGN(4K) : AT(ADDR (.data) - 0xC0000000)
	{
		*(.data)
	}
	
	/*uninitialised data*/
	.bss ALIGN(4K) : AT(ADDR (.bss) - 0xC0000000)
	{
		*(COMMON)
		*(.bss)
		
	}
	kernel_end = ALIGN(4K);
	
}
