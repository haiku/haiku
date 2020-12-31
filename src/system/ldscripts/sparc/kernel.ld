OUTPUT_FORMAT("elf64-sparc")
OUTPUT_ARCH(sparc:v9)

ENTRY(_start)

PHDRS
{
	headers PT_PHDR PHDRS ;
	text PT_LOAD FILEHDR PHDRS FLAGS(5) ;
	data PT_LOAD ;
	dynamic PT_DYNAMIC ;
}

SECTIONS
{
	. = 0xFFFFFFFF80000000 + SIZEOF_HEADERS;

	.interp : { *(.interp) } : text
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
	.init : { *(.init) } =0x90909090
	.plt : { *(.plt) }

	/* text/read-only data */
	.text :	{ *(.text .text.* .gnu.linkonce.t.*) } :text =0x90909090

	.rodata : { *(.rodata .rodata.* .gnu.linkonce.r.*) }

	. = ALIGN(0x8);
	altcodepatch_begin = .;
	.altcodepatch : { *(.altcodepatch) }
	altcodepatch_end = .;

	/* writable data - in a separate page for memory protection purposes */
	. = ALIGN (0x2000);
	__data_start = .;
	.data : { *(.data .data.* .gnu.linkonce.d.*) } :data

	. = ALIGN(0x4);
	__ctor_list = .;
	.ctors : { *(.ctors) }
	__ctor_end = .;
	__dtor_list = .;
	.dtors : { *(.dtors) }
	__dtor_end = .;
	.got : { *(.got.plt) *(.got) }
	.dynamic : { *(.dynamic) } :dynamic :data
	
	/* uninitialized data (in same segment as writable data) */
	. = ALIGN(0x8);
	__bss_start = .;
	.bss : { *(.bss .bss.* .gnu.linkonce.b.*) } :data

	/* make sure to end on a page boundary */
	. = ALIGN(0x2000);
	_end = . ;

	/* Strip unnecessary stuff */
	/DISCARD/ : { *(.comment .note .eh_frame) }
}
