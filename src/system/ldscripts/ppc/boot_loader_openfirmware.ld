OUTPUT_FORMAT("elf32-powerpc", "elf32-powerpc", "elf32-powerpc")
OUTPUT_ARCH(powerpc)

ENTRY(_start)

SECTIONS
{
	. = 0x202000 + SIZEOF_HEADERS;

	__text_begin = .;

	/* text/read-only data */
	.text :	{ *(.text .text.* .gnu.linkonce.t.*)
	          *(.rodata .rodata.* .gnu.linkonce.r.*)
	          *(.sdata2) }

	/* writable data  */
	/* align to the same offset in the next page (for performance reasons
	   (not that it really matters in the boot loader)) */
	. = ALIGN(0x1000) + 0x1000 + (. & (0x1000 - 1));

	.data : {
		__ctor_list = .;
		*(.ctors)
		__ctor_end = .;

		__data_start = .;
		*(.data .gnu.linkonce.d.*)
		*(.data.rel.ro.local .data.rel.ro*)
		*(.got .got2)
		*(.sdata .sdata.* .gnu.linkonce.s.* .fixup) }

	/* uninitialized data (in same segment as writable data) */
	__bss_start = .;
	.bss : { *(.sbss .sbss.* .gnu.linkonce.sb.*)

		*(.bss .bss.* .gnu.linkonce.b.*)
		. = ALIGN(0x1000);
	}

	_end = . ;

	/* Strip unnecessary stuff */
	/DISCARD/ : { *(.comment .note .eh_frame .dtors .debug_* .gnu.attributes) }
}
