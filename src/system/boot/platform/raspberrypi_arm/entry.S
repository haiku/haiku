.globl _start
_start:
	b boot

.balign 0x20
boot:
	mov r0, #0
	@ Machine ID 3138 (0xC42)
	mov r1, #66
	orr r1, r1, #3072

	ldr pc, kern_addr

kern_addr:
	.word kern

.balign 0x8000, 0
kern:
	/* Set up 1MB C Stack Space */
	mov sp, #0x100000
	mov r4, #0

	/* Start Haiku loader */
	b pi_start
