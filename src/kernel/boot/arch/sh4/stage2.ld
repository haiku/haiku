OUTPUT_FORMAT("elf32-shl", "elf32-shl", "elf32-shl")
OUTPUT_ARCH(sh)
  PHDRS 
  {
  	text PT_LOAD;
  	data PT_LOAD;
  }
  
ENTRY(__start)
SECTIONS
{
/*	. = 0x8c100000 + 0x1200 + SIZEOF_HEADERS; */
	. = 0x8c000000 + 0x2000 + 0x80;

	/* text/read-only data */
	.text :	{ *(.text .gnu.linkonce.t.*) }

	.rodata : { *(.rodata) }

	/* writable data  */
/*	. = ALIGN(0x1000);*/
	__ctor_list = .;
	.ctors : { *(.ctors) }
	__ctor_end = .;

	__data_start = .;
	.data : { *(.data .gnu.linkonce.d.*) }

	/* unintialized data (in same segment as writable data) */
	__bss_start = .;
	.bss : { *(.bss) }

	. = ALIGN(0x1000);
	_end = . ;

	/* Strip unnecessary stuff */
	/DISCARD/ : { *(.comment .note .eh_frame .dtors) }
}
