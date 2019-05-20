OUTPUT_FORMAT("elf64-sparc")
OUTPUT_ARCH(sparc:v9)

ENTRY(_start)

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
	          *(.sdata2) }

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
