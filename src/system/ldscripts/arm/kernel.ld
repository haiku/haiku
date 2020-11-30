OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)

ENTRY(_start)
SECTIONS
{
	. = 0x80000000 + SIZEOF_HEADERS;

	.interp : { *(.interp) }
	.hash : { *(.hash) }
	.dynsym : { *(.dynsym) }
	.dynstr : { *(.dynstr) }
	.rela.text : { *(.rela.text .rela.text.* .rela.gnu.linkonce.t.*) }
	.rela.data : { *(.rela.data .rela.data.* .rela.gnu.linkonce.d.*) }
	.rela.rodata : { *(.rela.rodata .rela.rodata.* .rela.gnu.linkonce.r.*) }
	.rela.got : { *(.rela.got) }
	.rela.ctors : { *(.rela.ctors) }
	.rela.dtors : { *(.rela.dtors) }
	.rela.init : { *(.rela.init) }
	.rela.fini : { *(.rela.fini) }
	.rela.bss : { *(.rela.bss .rela.bss.* .rela.gnu.linkonce.b.*) }
	.rela.plt : { *(.rela.plt) }
	.init : { *(.init) } =0x9090
	.plt : { *(.plt) }

	/* text/read-only data */
	.text :	{ *(.text .text.* .gnu.linkonce.t.*) } =0x9090

	.rodata : { *(.rodata .rodata.* .gnu.linkonce.r.*) }

	/*
	. = ALIGN(0x8);
	altcodepatch_begin = .;
	.altcodepatch : { *(.altcodepatch) }
	altcodepatch_end = .;
	*/

	/* writable data */
	. = ALIGN (0x1000);
	__data_start = .;
	.data : { *(.data .data.* .gnu.linkonce.d.*) }

	. = ALIGN(0x8);
	__ctor_list = .;
	.ctors : { *(.init_array) *(.ctors) }
	__ctor_end = .;
	__dtor_list = .;
	.dtors : { *(.fini_array) *(.dtors) }
	__dtor_end = .;
	.got : { *(.got.plt) *(.got) }
	.dynamic : { *(.dynamic) }

	/* uninitialized data (in same segment as writable data) */
	__bss_start = .;
	.bss : { *(.bss .bss.* .gnu.linkonce.b.*) }

	. = ALIGN(0x1000);
	_end = . ;

	/* Strip unnecessary stuff */
	/DISCARD/ : { *(.comment .note .eh_frame) }
}
