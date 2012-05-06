.globl _start
_start:
	b jmp_loader

.balign 0x8000, 0
jmp_loader:
	/* Set up 1MB C Stack Space */
	mov sp, #0x100000
	mov r4, #0

	/* Start Haiku loader */
	b pi_start
