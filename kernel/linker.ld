ENTRY(_loader)
/*OUTPUT_FORMAT(elf32-i686)*/
LOAD_ADDR =	   0x00100000;
VIRTUAL_BASE = 0xC0000000;


SECTIONS {
	. = LOAD_ADDR + VIRTUAL_BASE;
	_kernel_start = .;

	.text ALIGN(0x1000) : AT(ADDR(.text) - VIRTUAL_BASE)	{
		*(.multiboot)
		*(.text)
	}

	.rodata ALIGN(0x1000) : AT(ADDR(.rodata) - VIRTUAL_BASE)	{
		*(.rodata)
	}

	.data ALIGN(0x1000) : AT(ADDR(.data) - VIRTUAL_BASE)	{
		*(.data)
	}

	.bss ALIGN(0x1000) : AT(ADDR(.bss) - VIRTUAL_BASE) {
		*(COMMON)
		*(.bss)
	}

	_kernel_end = .;
}
