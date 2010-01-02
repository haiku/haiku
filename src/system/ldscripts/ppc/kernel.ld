OUTPUT_FORMAT("elf32-powerpc", "elf32-powerpc", "elf32-powerpc")
OUTPUT_ARCH(powerpc)

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

	.rel.dyn : {
		*(.rel.init)
		*(.rel.text .rel.text.* .rel.gnu.linkonce.t*)
		*(.rel.fini)
		*(.rel.rodata .rel.rodata.* .rel.gnu.linkonce.r*)
		*(.rel.data.rel.ro*)
		*(.rel.data .rel.data.* .rel.gnu.linkonce.d.*)
		*(.rel.tdata .rel.tdata.* .rel.gnu.linkonce.td.*)
		*(.rel.ctors)
		*(.rel.dtors)
		*(.rel.got)
		*(.rel.sdata .rel.sdata.* .rel.gnu.linkonce.s.*)
		*(.rel.sbss .rel.sbss.* .rel.gnu.linkonce.sb.*)
		*(.rel.sdata2 .rel.sdata2.* .rel.gnu.linkonce.s2.*)
		*(.rel.sbss2 .rel.sbss2.* .rel.gnu.linkonce.sb2.*)
		*(.rel.bss .rel.bss.* .rel.gnu.linkonce.b.*)
	}

	.rela.dyn : {
		*(.rela.init)
		*(.rela.text .rela.text.* .rela.gnu.linkonce.t*)
		*(.rela.fini)
		*(.rela.rodata .rela.rodata.* .rela.gnu.linkonce.r*)
		*(.rela.data .rela.data.* .rela.gnu.linkonce.d.*)
		*(.rela.tdata .rela.tdata.* .rela.gnu.linkonce.td.*)
		*(.rela.ctors)
		*(.rela.dtors)
		*(.rela.got)
		*(.rela.got1)
		*(.rela.got2)
		*(.rela.sdata .rela.sdata.* .rela.gnu.linkonce.s.*)
		*(.rela.sbss .rela.sbss.* .rela.gnu.linkonce.sb.*)
		*(.rela.sdata2 .rela.sdata2.* .rela.gnu.linkonce.s2.*)
		*(.rela.sbss2 .rela.sbss2.* .rela.gnu.linkonce.sb2.*)
		*(.rela.bss .rela.bss.* .rela.gnu.linkonce.b.*)
	}
	.init : { *(.init) } =0x60000000

	/* text/read-only data */
	.text :	{
		*(.text .stub .text.* .gnu.linkonce.t.*)
	} =0x60000000

	.rodata : { *(.rodata .rodata.* .gnu.linkonce.r.*) }
	.rodata1 : { *(.rodata1) }
	.sdata2 : { *(.sdata2 .sdata2.* .gnu.linkonce.s2.*) }
	.sbss2 : { *(.sbss2 .sbss2.* .gnu.linkonce.sb2.*) }

	/* writable data  */
	/* align to the same offset in the next page (for performance reasons
	   (not that it really matters in the kernel)) */
	. = ALIGN(0x1000) + 0x1000 + (. & (0x1000 - 1));

	__ctor_list = .;
	.ctors : { *(.ctors) }
	__ctor_end = .;

	.dynamic : { *(.dynamic) }

	__data_start = .;
	.data : { *(.data .gnu.linkonce.d.*) }

	.got : { *(.got.plt) *(.got) }
	
	.sdata : { *(.sdata .sdata.* .gnu.linkonce.s.*) }

	/* uninitialized data (in same segment as writable data) */
	__bss_start = .;
	.sbss : { *(.sbss .sbss.* .gnu.linkonce.sb.*) }
	.bss : {
		*(.bss .bss.* .gnu.linkonce.b.*)
		. = ALIGN(0x1000);
	}

	_end = . ;


	/* Strip unnecessary stuff */
	/DISCARD/ : { *(.comment .note .eh_frame .dtors .debug_*) }
}
