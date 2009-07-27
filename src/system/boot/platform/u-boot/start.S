#include <arch/arm/arch_cpu.h>

#include <asm_defs.h>


/*
 * Entry point to the loader that U-Boot passes control to.
 */
	.text
	.globl	_start
_start:
	/* I'm to stupid to get _start in a c file to work ..... */
	b	start2
