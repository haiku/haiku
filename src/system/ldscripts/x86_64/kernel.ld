OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH("i386:x86-64")

ENTRY(_start)
SECTIONS
{
	. = 0xFFFFFFFF80000000 + SIZEOF_HEADERS;

	.interp : { *(.interp) }
	.hash : { *(.hash) }
	.dynsym : { *(.dynsym) }
	.dynstr : { *(.dynstr) }
	.gnu.version : { *(.gnu.version) }
	.gnu.version_d : { *(.gnu.version_d) }
	.gnu.version_r : { *(.gnu.version_r) }
	.init : { *(.init) } =0x90909090

	/* text/read-only data */
	.text :	{ *(.text .text.* .gnu.linkonce.t.*) } =0x90909090

	.rodata : { *(.rodata .rodata.* .gnu.linkonce.r.*) }

	. = ALIGN(0x8);
	altcodepatch_begin = .;
	.altcodepatch : { *(.altcodepatch) }
	altcodepatch_end = .;

	.gcc_except_table : ONLY_IF_RO { *(.gcc_except_table .gcc_except_table.*) }

	/* writable data */
	. = ALIGN (0x1000);
	__data_start = .;
	.data : { *(.data .data.* .gnu.linkonce.d.*) }

	_haiku_revision : { *(_haiku_revision) }

	. = ALIGN(0x4);
	__ctor_list = .;
	.ctors : { *(.ctors) }
	__ctor_end = .;
	__dtor_list = .;
	.dtors : { *(.dtors) }
	__dtor_end = .;
	.dynamic : { *(.dynamic) }
	
	/* uninitialized data (in same segment as writable data) */
	.bss : {
		__bss_start = .;
		*(.dynbss)
		*(.bss .bss.* .gnu.linkonce.b.*)
	}
	. = ALIGN(0x1000);
	_end = . ;

	/* Debug  */
	INCLUDE debuginfo.ld

	/* Strip unnecessary stuff */
	/DISCARD/ : { *(.comment .note .eh_frame .sframe) }

	.got : { *(.got) *(.got.plt) *(.igot.*) }
	ASSERT(SIZEOF(.got) == 0, "Unexpected .got found!")

	.plt : { *(.plt) *(.plt.*) *(.iplt) }
	ASSERT(SIZEOF(.plt) == 0, "Unexpected .plt found!")

	.rela : { *(.rela.*) }
	ASSERT(SIZEOF(.rela) == 0, "Unexpected .rela found!")
}
