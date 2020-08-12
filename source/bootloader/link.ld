ENTRY(_start)

SECTIONS {
	/* take care of positioning problems*/
	. = 0x7C00;
	
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
