OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)

ENTRY(runtime_loader)
SEARCH_DIR("libgcc");
SECTIONS
{
	. = 0x00200000 + SIZEOF_HEADERS;

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
	.init : { *(.init) } =0x90909090
	.plt : { *(.plt) }

	/* text/read-only data */
	.text :	{ *(.text .text.* .gnu.linkonce.t.*) } =0x90909090

	.rodata : { *(.rodata .rodata.* .gnu.linkonce.r.*) }

	/* writable data  */
	. = ALIGN (CONSTANT (MAXPAGESIZE)) + (. & (CONSTANT (MAXPAGESIZE) - 1));
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
	/DISCARD/ : { *(.comment .note .eh_frame .dtors) }
}
