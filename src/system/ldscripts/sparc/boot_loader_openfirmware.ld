OUTPUT_FORMAT("elf64-sparc")
OUTPUT_ARCH(sparc:v9)

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
	/* Execution address. The file is loaded at 0x4000 with its header,
	 * then relocated here. */
	. = 0x202000;
	__text_begin = .;

	/* keep text and data sections next to each other, as the loader in openboot
	 * is not able to handle a hole between them.
	 */

	/* text/read-only data */
	.text :	{ 
			  /* Make sure entry point is at start of file. Not required, since
			   * it is set using ENTRY above, but it looks nicer and makes it
			   * clearer we jumped at the correct address. */
              *(.text.start)

			  *(.text .text.* .gnu.linkonce.t.*)
	          *(.rodata .rodata.* .gnu.linkonce.r.*)
	          *(.sdata2) } :text

	.data : {
		__ctor_list = .;
		*(.ctors)
		__ctor_end = .;

		__data_start = .;
		*(.data .gnu.linkonce.d.*)
		*(.data.rel.ro.local .data.rel.ro*)
		*(.got .got2)
		*(.sdata .sdata.* .gnu.linkonce.s.* .fixup) } :text

	/* uninitialized data (in same segment as writable data) */
	__bss_start = .;
	.bss : { *(.sbss .sbss.* .gnu.linkonce.sb.*)

		*(.bss .bss.* .gnu.linkonce.b.*)
		. = ALIGN(0x1000);
	} :text

	_end = . ;

	/* Strip unnecessary stuff */
	/DISCARD/ : { *(.comment .note .eh_frame .dtors .debug_* .gnu.attributes) }
}
