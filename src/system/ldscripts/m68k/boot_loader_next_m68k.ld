OUTPUT_FORMAT("elf32-m68k", "elf32-m68k", "elf32-m68k")
OUTPUT_ARCH(m68k)

ENTRY(_start)

PHDRS
{
	/* have a non-loadable program chunk with the headers, then a loadable one
	 * with the actual data. This eases the work of elf2aout. */
	headers PT_NULL FILEHDR PHDRS ;
	text PT_LOAD ;
}

SECTIONS
{
	/* Execution address.
	 * The NetBSD loader uses this, not sure if it's a ROM constant or not.
	 */
	. = 0x04380000;
	__text_begin = .;

	/* text/read-only data */
	.text :	{
			/* Make sure entry point is at start of file. Not required, since
			 * it is set using ENTRY above, but it looks nicer and makes it
			 * clearer we jumped at the correct address. */
			*(.text.start)

			*(.text .text.* .gnu.linkonce.t.*)
			*(.rodata .rodata.* .gnu.linkonce.r.*)
			*(.sdata2)
	} :text

	/*. = ALIGN(0x4);*/
	.data : {
		__ctor_list = .;
		*(.ctors)
		__ctor_end = .;

		__data_start = .;
		*(.data .gnu.linkonce.d.*)
		*(.data.rel.ro.local .data.rel.ro*)
		*(.got .got2)
		*(.sdata .sdata.* .gnu.linkonce.s.* .fixup)
	} :text

	/* uninitialized data (in same segment as writable data) */
	__bss_start = .;
	.bss : {
		*(.sbss .sbss.* .gnu.linkonce.sb.*)
		*(.bss .bss.* .gnu.linkonce.b.*)
		. = ALIGN(0x1000);
	} :text

	_end = . ;

	/* Strip unnecessary stuff */
	/DISCARD/ : { *(.comment .note .eh_frame .dtors .stab .stabstr .debug* .debug_* .gnu.attributes) }
}
