.section .init
.globl _entry
_entry:
	b jmp_loader

.section .text
	/* Start "safe" code area.
	 * 0x4000 minimum safe area
	 * defined in boot_loader_raspberrypi_arm.ld
	 */

jmp_loader:
	/* Start Haiku loader */

	/* For Thumb code? */
	/*
	ldr r12, =_start
	mov lr, pc
	bx r12
	*/

	b _start

	/* Cause exception if loader returns */
	swi 0xFFFFFF
