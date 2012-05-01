.globl _start
_start:

	/* Set up 1MB C Stack Space */
	mov sp, #0x100000

	mov r4,#0
	/* Start loader */
	b pi_start
