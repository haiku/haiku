OUTPUT_FORMAT("elf32-powerpc", "elf32-powerpc", "elf32-powerpc")
OUTPUT_ARCH(powerpc)

ENTRY(_start)

SECTIONS
{
	. = 0x102000 + SIZEOF_HEADERS;

	/* text/read-only data */
	.text :	{ *(.text .gnu.linkonce.t.*) }

	.rodata : { *(.rodata .rodata.* .gnu.linkonce.r.*) }
	.sdata2 : { *(.sdata2) }

	/* writable data  */
	. = ALIGN(0x1000);

	__ctor_list = .;
	.ctors : { *(.ctors) }
	__ctor_end = .;

	__data_start = .;
	.data : { *(.data .gnu.linkonce.d.*) }
	.data.rel.ro : { *(.data.rel.ro.local .data.rel.ro*) }
	.got : { *(.got .got2) }
	.sdata : { *(.sdata .sdata.* .gnu.linkonce.s.*) }

	/* unintialized data (in same segment as writable data) */
	__bss_start = .;
	.sbss : { *(.sbss .sbss.* .gnu.linkonce.sb.*) }
	.bss : {
		*(.bss .bss.* .gnu.linkonce.b.*)
		. = ALIGN(0x1000);
	}

	_end = . ;

	/* Strip unnecessary stuff */
	/DISCARD/ : { *(.comment .note .eh_frame .dtors .debug_*) }
}
