.globl _start
_start:
	b jmp_loader

.balign 0x8000, 0
jmp_loader:
	/* Start Haiku loader */

	/* For Thumb code? */
	/*
	ldr r12, =pi_start
	mov lr, pc
	bx r12
	*/

	b pi_start

	/* Cause exception if loader returns */
	swi 0xFFFFFF
