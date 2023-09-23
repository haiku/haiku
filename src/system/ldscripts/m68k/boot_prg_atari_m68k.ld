/*
 * ld script to generate a TOS .PRG
 * *ONLY* works for PIC (PCREL) code!
 */

OUTPUT_FORMAT("binary", "binary", "binary")
OUTPUT_ARCH(m68k)

ENTRY(__text_start)
SEARCH_DIR("libgcc");
SECTIONS
{
	/*. = 0x80000000 + SIZEOF_HEADERS;*/
	/* fool the alignment of .text */
	/*. = 0x20 - 0x1c;*/
	/* should stay in sync with boot_loader_m68k_atari.ld and atari_memory_map.h */
	. = 0x00080000 - 0x1c ;

	/* .PRG file header */
	.prgheader : {
	/*SHORT(0x55aa)*/
		SHORT(0x601a)				/* MAGIC */
		LONG(__data_start - __text_start)	/* TEXT SIZE */
		LONG(__bss_start - __data_start)	/* DATA SIZE */
		LONG(_end - __bss_start)		/* BSS SIZE */
		LONG(0)					/* SYMBOL TABLE SIZE */
		LONG(0x4841494b)			/* (reserved) 'HAIK' */
		LONG(0x1)				/* PRGFLAGS : PF_FASTLOAD */
		SHORT(1)				/* ABSFLAG */
	}

	__text_start = .;
	/* ".text ." should align to 0x1c but doesn't work as expected, cf top. */
	.text : { 
		*(.text .gnu.linkonce.t.*) 
	/*	*(.rel.text) *(.rel.gnu.linkonce.t*) 
		*(.rela.text) *(.rela.gnu.linkonce.t*) */
	}
	. = ALIGN(0x4);
	__ctor_list = .;
	.ctors : { *(.ctors) }
	__ctor_end = .;

	.rodata : {
		*(.rodata) 
	/*	*(.rel.rodata) *(.rel.gnu.linkonce.r*)
		*(.rela.rodata) *(.rela.gnu.linkonce.r*) */
	}
	_haiku_revision : { *(_haiku_revision) }

	/* writable data  */
	/* NO! . = ALIGN(0x1000); */
	. = ALIGN(0x1000);
	__data_start = .;
	.data : {
		*(.data .gnu.linkonce.d.*)
	/*	*(.rel.data) *(.rel.gnu.linkonce.d*)
		*(.rela.data) *(.rela.gnu.linkonce.d*) */
	}
	/* uninitialized data (in same segment as writable data) */
	__bss_start = .;
	.bss : {
		*(.bss)
	/*	*(.rel.bss)
		*(.rela.bss) */
	}
	. = ALIGN(0x1000);
	_end = . ;
	.prgtrailer : {
		LONG(0)					/* FIXUP OFFSET */
		LONG(0)
	}
	/* Strip unnecessary stuff */
	/DISCARD/ : { *(.comment .note .eh_frame) }
}

