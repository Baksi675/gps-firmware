ENTRY(Reset_Handler)

MEMORY {
	rom (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
	ram (rw!x) : ORIGIN = 0x20000000, LENGTH = 96K
}

SECTIONS {
	.vector : {
		*(.vector)
		. = ALIGN(4);
	} > rom

	.text : {
		*(.text)
		. = ALIGN(4);
	} > rom

	.rodata : {
		*(.rodata)
		. = ALIGN(4);
	} > rom

	.bss : {
		__bss_start = .;
		*(.bss)
		. = ALIGN(4);
		__bss_end = .;
	} > ram

	__data_lma = LOADADDR(.data);

	.data : {
		__data_start = .;
		*(.data)
		. = ALIGN(4);
		__data_end = .;
	} > ram AT> rom
}
