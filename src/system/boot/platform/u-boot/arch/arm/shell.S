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
	mov	r0,#0
	b	_start_common
SYMBOL_END(_start_raw)
SYMBOL_END(_start)

/*
 * called from bootm with netbsd loader compatible args
 */
SYMBOL(_start_netbsd):
	mov	r0,#1
	b	_start_common
SYMBOL_END(_start_netbsd)



SYMBOL(_start_common):
	strb	r0,gUBootOS
	str	r8,gUBootGlobalData
	/* ... */
	ldrb	r0,gUBootOS
	cmp	r0,#0
	beq	start_raw
	cmp	r0,#1
	beq	start_netbsd
	mov	pc,lr
SYMBOL_END(_start_common)




SYMBOL(gUBootGlobalData):
	.long   0
SYMBOL_END(gUBootGlobalData)
SYMBOL(gUBootOS):
	.byte   0
SYMBOL_END(gUBootOS)

