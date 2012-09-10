OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

C_STACK_SIZE	= 1048576;
IRQ_STACK_SIZE	= 0;
FIQ_STACK_SIZE	= 0;
SVC_STACK_SIZE	= 0;
ABT_STACK_SIZE	= 0;
UND_STACK_SIZE	= 0;

SECTIONS
{
	/*
	 * Raspberry Pi boot logic:
	 * start.elf loads the kernel specified in config.txt
	 * if a dtb exists, it is loaded at 0x100 and the kernel at 0x8000
	 * else if disable_commandline_tags true, set kernel load address to 0x0
	 * else load kernel at 0x8000 and put atags at 0x100
	 */

	. = BOARD_LOADER_BASE;

	/* .init is a stub that jumps to code at .text */
	.init : {
		*(.init)
	}

	.text 0x8000 : {
		CREATE_OBJECT_SYMBOLS
		*(.text .text.* .gnu.linkonce.t.*)
		*(.plt)
		*(.gnu.warning)
	}

	. = ALIGN(0x4);
	.ctors : {
		__ctor_list = .;
		*(.init_array);
		*(.ctors);
		__ctor_end = .;
	}

	.dtors : {
		__dtor_list = .;
		*(.fini_array);
		*(.dtors);
		__dtor_end = .;
	}

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
	.bss : {
		__bss_start = .;
		*(.shbss)
		*(.bss .bss.* .gnu.linkonce.b.*)
		*(COMMON)
		. = ALIGN(0x4);
		__bss_end = .;
	}

	.stack : {
		__stack_start = .;

		. += IRQ_STACK_SIZE;
		. = ALIGN(0x4);
		__irq_stack_top = .;

		. += FIQ_STACK_SIZE;
		. = ALIGN(0x4);
		__fiq_stack_top = .;

		. += SVC_STACK_SIZE;
		. = ALIGN(0x4);
		__svc_stack_top = .;

		. += ABT_STACK_SIZE;
		. = ALIGN(0x4);
		__abt_stack_top = .;

		. += UND_STACK_SIZE;
		. = ALIGN(0x4);
		__und_stack_top = .;

		. += C_STACK_SIZE;
		. = ALIGN(0x4);
		__c_stack_top = .;

		__stack_end = .;
	}

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
}
