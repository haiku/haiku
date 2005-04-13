OUTPUT_FORMAT("elf32-bigmips", "elf32-bigmips", "elf32-bigmips")
OUTPUT_ARCH(mips)

ENTRY(_start)
SECTIONS
{

	/* this is the starting address the SGI Indy puts the image at
	+ the size of the stage1 bootloader
	+ the size of the bootdir
	+ the size of the program headers */
	. = 0x88002000 + 0x2000 + SIZEOF_HEADERS;

	/* text/read-only data */
	.text :	{ *(.text .gnu.linkonce.t.*) }

	__ctor_list = .;
	.ctors : { *(.ctors) }
	__ctor_end = .;

	.rodata :
	{ 
		*(.rodata)
		. = ALIGN(0x1000);
	} =0x9000

	/* writable data  */
	__data_start = .;
	.data : { *(.data .gnu.linkonce.d.*) }

	/* unintialized data (in same segment as writable data) */
	__bss_start = .;
	.bss : { *(.bss) }

	. = ALIGN(0x1000); 
	_end = . ;

	/* Strip unnecessary stuff */
	/DISCARD/ : { *(.reginfo .comment .note .eh_frame .dtors) }
}
