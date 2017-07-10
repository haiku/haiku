OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)

STACK_SIZE = 16384;

ENTRY(_start)
SECTIONS
{
	. = BOOT_LOADER_BASE;

	/* text/read-only data */
	.text :	{ *(.text .gnu.linkonce.t.*) }

	. = ALIGN(0x4);
	__ctor_list = .;
	.ctors : { *(.ctors) *(.init_array) }
	__ctor_end = .;

	.rodata : { *(.rodata .rodata.*) }

	/* writable data  */
	. = ALIGN(0x1000);
	__data_start = .;
	.data : { *(.data .gnu.linkonce.d.*) }

	/* exception unwinding - should really not be needed! */
	__exidx_start = .;
	.ARM.exidx   : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) }
	__exidx_end = .;

	/* uninitialized data (in same segment as writable data) */
	__bss_start = .;
	.bss : { *(.bss) }
	__bss_end = .;

	.stack : {
		__stack_start = .;
		. += STACK_SIZE;
		. = ALIGN(0x8);
		__stack_top = .;
	}

	. = ALIGN(0x1000);
	_end = . ;
  /* Stabs debugging sections.  */
  .stab 0 : { *(.stab) }
  .stabstr 0 : { *(.stabstr) }
  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to the beginning
     of the section so we begin them at 0.  */
  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }
  /* These must appear regardless of  .  */

	/* Strip unnecessary stuff */
/*	/DISCARD/ : { *(.comment .note .eh_frame .dtors .stab .stabstr .debug*) }*/
}
