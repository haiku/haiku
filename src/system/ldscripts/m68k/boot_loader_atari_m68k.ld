OUTPUT_FORMAT("elf32-m68k", "elf32-m68k", "elf32-m68k")
OUTPUT_ARCH(m68k)

ENTRY(_start)
SECTIONS
{
	//. = 0x10000;
	. = 0x00080000;

	/* text/read-only data */
	.text :	{ *(.text .text.* .gnu.linkonce.t.*) }

	. = ALIGN(0x4);
	__ctor_list = .;
	.ctors : { *(.ctors) }
	__ctor_end = .;

	.rodata : { *(.rodata .rodata.*) }
	_haiku_revision : { *(_haiku_revision) }

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
