/*this should be used when linking stage2. */
ENTRY(_stage2)

SECTIONS {
	/* this is where stage1 loads it.*/
	. = 0x7e00;
	
	.text : ALIGN(512)
	{
		*(.text)
		*(.data)
		*(.rodata)
	}
	
	.bss : ALIGN(512)
	{
		*(.bss)
		*(COMMON)
	}
	
}
