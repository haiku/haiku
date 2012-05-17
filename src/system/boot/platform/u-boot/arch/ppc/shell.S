//#include <arch/ppc/arch_cpu.h>

#include <asm_defs.h>


	.text

/*
 * Entry points to the loader that U-Boot passes control to.
 */

/*
 * called as Linux kernel entry
 * *MUST* be first symbol
 */
SYMBOL(_start_linux):
/*
 * ELF entry
 */
SYMBOL(_start):

	li	%r11, 0
	b	_start_common

SYMBOL_END(_start_linux)
SYMBOL_END(_start)





SYMBOL(_start_common):

	lis		%r12, gUBootGlobalData@ha
	ori		%r12, %r12, gUBootGlobalData@l
	stw		%r2, 0(%r12)
	stw		%r11, 8(%r12)	// gUBootOS

	b		start_linux_ppc_fdt

	// return
	blr
SYMBOL_END(_start_common)


//XXX: doesn't seem to work
//.data


SYMBOL(gUBootGlobalData):
	.long   0
SYMBOL_END(gUBootGlobalData)
SYMBOL(gUImage):
	.long   0
SYMBOL_END(gUImage)
SYMBOL(gUBootOS):
//XXX: bug? Using .byte makes the next asm symbol
// to be at the same address
//	.byte	0
	.long   0
SYMBOL_END(gUBootOS)

