#include <arch/arm/arch_cpu.h>

#include <asm_defs.h>


	.text

/*
 * Entry points to the loader that U-Boot passes control to.
 */

/*
 * called as standalone (raw binary)
 * *MUST* be first symbol
 */
SYMBOL(_start_raw):
/*
 * ELF entry
 */
SYMBOL(_start):
	mov	r4,#0
	b	_start_common
SYMBOL_END(_start_raw)
SYMBOL_END(_start)

/*
 * called from bootm with netbsd loader compatible args
 */
SYMBOL(_start_netbsd):
	mov	r4,#1
	b	_start_common
SYMBOL_END(_start_netbsd)



SYMBOL(_start_common):
	strb	r4,gUBootOS
	str	r8,gUBootGlobalData
	/* ... */
	ldrb	r4,gUBootOS
	cmp	r4,#0
	beq	start_raw
	cmp	r4,#1
	beq	start_netbsd
	mov	pc,lr
SYMBOL_END(_start_common)




SYMBOL(gUBootGlobalData):
	.long   0
SYMBOL_END(gUBootGlobalData)
SYMBOL(gUImage):
	.long   0
SYMBOL_END(gUImage)
SYMBOL(gUBootOS):
	.byte   0
SYMBOL_END(gUBootOS)

