OUTPUT_FORMAT("elf32-bigmips", "elf32-bigmips", "elf32-bigmips")
OUTPUT_ARCH(mips)

PHDRS 
{
	text PT_LOAD;
	data PT_LOAD;
}

ENTRY(__start)
SECTIONS
{

	. = 0xc0000000 + SIZEOF_HEADERS;

	/* text/read-only data */
	.text :	{ *(.text .gnu.linkonce.t.*) } :text

	__ctor_list = .;
	.ctors : { *(.ctors) } :text
	__ctor_end = .;

	.rodata :
	{ 
		*(.rodata)
		. = ALIGN(0x1000);
	} =0x9000

	/* writable data  */
	__data_start = .;
	.data : { *(.data .gnu.linkonce.d.*) } :data

	/* unintialized data (in same segment as writable data) */
	__bss_start = .;
	.bss : { *(.bss) } :data

	. = ALIGN(0x1000); 
	_end = . ;

	/* Strip unnecessary stuff */
	/DISCARD/ : { *(.comment .note .eh_frame .dtors) }
}
