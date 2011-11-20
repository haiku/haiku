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

	/*
	 * Turn off interrupts and make sure we are in SVC mode
	 */

	mrs	r4, cpsr
	bic	r4, r4, #0x1f
	orr	r4, r4, #0xd3
	msr	cpsr, r4

	/*
	 * Turn off MMU and data cache if necessary.
	 * WARNING: assumes we are running with a 1-1 mapping if MMU is enabled.
	 */
	mrc	p15, 0, r4, c1, c0, 0
	bic	r4, r4, #0x0000000f		     // WCAM bits
	bic	r4, r4, #0x00000300		     // RS   bits
	mcr	p15, 0, r4, c1, c0, 0
	mov	r4, #0
	mov	r4, r4
	mov	r4, r4




	ldrb	r4,gUBootOS
	cmp	r4,#0
	beq	start_raw
	cmp	r4,#1
	beq	start_netbsd
	mov	pc,lr
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

