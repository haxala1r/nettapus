ENTRY(_start)

SECTIONS {
	
	/* 1M is a conventional place to begin, apparently.*/
	. = 1M;
	
	
	
	.text BLOCK(4K) : ALIGN(4K)
	{
		kernel_start = .;
		*(.multiboot)
		
		*(.text)
	}
	/*read-only data*/
	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}
	/*initialised data*/
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}
	
	/*uninitialised data*/
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
		
	}
	kernel_end = ALIGN(4K);
	
}
