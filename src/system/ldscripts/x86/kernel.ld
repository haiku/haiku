OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)

ENTRY(_start)
SEARCH_DIR("libgcc");
SECTIONS
{
	. = 0x80000000 + SIZEOF_HEADERS;

	.interp : { *(.interp) }
	.hash : { *(.hash) }
	.dynsym : { *(.dynsym) }
	.dynstr : { *(.dynstr) }
	.gnu.version : { *(.gnu.version) }
	.gnu.version_d : { *(.gnu.version_d) }
	.gnu.version_r : { *(.gnu.version_r) }
	.rel.text : { *(.rel.text .rel.text.* .rel.gnu.linkonce.t.*) }
	.rela.text : { *(.rela.text .rela.text.* .rela.gnu.linkonce.t.*) }
	.rel.data : { *(.rel.data .rel.data.* .rel.gnu.linkonce.d.*) }
	.rela.data : { *(.rela.data .rela.data.* .rela.gnu.linkonce.d.*) }
	.rel.rodata : { *(.rel.rodata .rel.rodata.* .rel.gnu.linkonce.r.*) }
	.rela.rodata : { *(.rela.rodata .rela.rodata.* .rela.gnu.linkonce.r.*) }
	.rel.data.rel.ro   : { *(.rel.data.rel.ro* .rel.gnu.linkonce.d.rel.ro.*) }
	.rel.got : { *(.rel.got) }
	.rela.got : { *(.rela.got) }
	.rel.ctors : { *(.rel.ctors) }
	.rela.ctors : { *(.rela.ctors) }
	.rel.dtors : { *(.rel.dtors) }
	.rela.dtors : { *(.rela.dtors) }
	.rel.init : { *(.rel.init) }
	.rela.init : { *(.rela.init) }
	.rel.fini : { *(.rel.fini) }
	.rela.fini : { *(.rela.fini) }
	.rel.bss : { *(.rel.bss) }
	.rela.bss : { *(.rela.bss) }
	.rel.plt : { *(.rel.plt) }
	.rela.plt : { *(.rela.plt) }
	.rel.ifunc : { *(.rel.ifunc) }
	.rel.altcodepatch : { *(.rel.altcodepatch) }

	.init : { *(.init) } =0x9090
	.plt : { *(.plt .iplt .plt.got) }

	/* text/read-only data */
	.text :	{ *(.text .text.* .gnu.linkonce.t.*) } =0x9090

	.rodata : { *(.rodata .rodata.* .gnu.linkonce.r.*) }

	. = ALIGN(0x4);
	altcodepatch_begin = .;
	.altcodepatch : { *(.altcodepatch) }
	altcodepatch_end = .;

	.gcc_except_table : ONLY_IF_RO { *(.gcc_except_table .gcc_except_table.*) }

	/* writable data  */
	. = ALIGN(0x1000);
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
	.got : { *(.got.plt) *(.got) }
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
	/DISCARD/ : { *(.comment .note .eh_frame .rel.eh_frame .sframe) }

	.iplt : { *(.rel.iplt .igot.plt) }
	ASSERT(SIZEOF(.iplt) == 0, "Unexpected .iplt found!")
}
