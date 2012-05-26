OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)

ENTRY(_start)
SECTIONS
{
	. = 0x10000;

	/* text/read-only data */
	.text :	{ *(.text .gnu.linkonce.t.*) }

	. = ALIGN(0x4);
	__ctor_list = .;
	.ctors : { *(.ctors) }
	__ctor_end = .;

	.rodata : { *(.rodata .rodata.*) }

	/* writable data  */
	. = ALIGN(0x1000);
	__data_start = .;
	.data : { *(.data .gnu.linkonce.d.*) }

	/* uninitialized data (in same segment as writable data) */
	__bss_start = .;
	.bss : { *(.bss) }

	. = ALIGN(0x1000);
	_end = . ;

	/* Strip unnecessary stuff */
	/DISCARD/ : { *(.comment .note .eh_frame .dtors .stab .stabstr .debug*) }
}
