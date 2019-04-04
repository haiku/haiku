OUTPUT_FORMAT("elf64-sparc")
OUTPUT_ARCH(sparc:v9)

ENTRY(runtime_loader)
SEARCH_DIR("libgcc");
SECTIONS
{
	. = 0x00100000 + SIZEOF_HEADERS;

	.interp : { *(.interp) }
	.hash : { *(.hash) }
	.dynsym : { *(.dynsym) }
	.dynstr : { *(.dynstr) }
	.rel.text : { *(.rel.text) *(.rel.gnu.linkonce.t*) }
	.rela.text : { *(.rela.text) *(.rela.gnu.linkonce.t*) }
	.rel.data : { *(.rel.data) *(.rel.gnu.linkonce.d*) }
	.rela.data : { *(.rela.data) *(.rela.gnu.linkonce.d*) }
	.rel.rodata : { *(.rel.rodata) *(.rel.gnu.linkonce.r*) }
	.rela.rodata : { *(.rela.rodata) *(.rela.gnu.linkonce.r*) }
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
	.init : { *(.init) } =0x9090
	.plt : { *(.plt) }

	/* text/read-only data */
	.text :	{ *(.text .gnu.linkonce.t.*) }

	.rodata : { *(.rodata) }

	/* writable data  */
  	. = ALIGN(0x1000) + (. & (0x1000 - 1));
	__data_start = .;
	PROVIDE(_data_start = .);
	.data : { *(.data .gnu.linkonce.d.*) }

	__ctor_list = .;
	PROVIDE (_ctor_list = .);
	.ctors : { *(.ctors) }
	PROVIDE (__ctor_end = .);


	/* uninitialized data (in same segment as writable data) */
	PROVIDE (__bss_start = .);
	.bss : { *(.bss) }

	. = ALIGN(0x1000);
	PROVIDE (_end = .);

	/* Strip unnecessary stuff */
	/DISCARD/ : { *(.comment .note .dtors) }
}
