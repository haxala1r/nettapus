ENTRY(_start)

SECTIONS {
	
	. = 0xffffffff80200000;
	
	
	.stivale2hdr : {
		kernel_start = .;
		KEEP(*(.stivale2hdr))
	}
	
	.text : {
		*(.text*)
	}
	
	.data : {
		*(.data*)
	}
	
	.rodata : {
		*(.rodata*)
	}
	
	.bss : {
		*(.COMMON)
		*(.bss*)
	}
	
	kernel_end = ALIGN(2M);
}
