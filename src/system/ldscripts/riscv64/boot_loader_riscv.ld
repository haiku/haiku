OUTPUT_FORMAT("elf64-littleriscv", "elf64-littleriscv", "elf64-littleriscv")
OUTPUT_ARCH(riscv)

ENTRY(_start)
SECTIONS
{
	. = 0x80000000;

	/* text/read-only data */
  .text : {
    *(.text.head)
    *(.text)
    *(.text.*)
    *(.gnu.linkonce.t.*)
  }

	. = ALIGN(0x8);
	__ctor_list = .;
	.ctors : { *(.ctors) }
	__ctor_end = .;

	.rodata : { *(.rodata .rodata.*) }

	/* writable data  */
	. = ALIGN(0x1000);
	__data_start = .;
	.data : {
		*(.data)
		*(.data.*)
		*(.gnu.linkonce.d.*)
	}
	.got : { *(.got.plt) *(.got) }
	.dynamic : { gDynamic = .; *(.dynamic) }

	/* uninitialized data (in same segment as writable data) */
	__bss_start = .;
	.bss : { *(.bss) }

	. = ALIGN(0x1000);
	_end = . ;

	.bss.stack : { *(.bss.stack) }

	/* Strip unnecessary stuff */
	/DISCARD/ : { *(.comment .note .eh_frame .dtors .stab .stabstr .debug*) }
}
