	.globl _start
	#
	# NOTE:	Registers r10-r11 are reserved for the interrupt handler
	#	while the others can be used by the main loop/start code.

_start:		
	# patch the external interrupt handlers entry
	add r1, r0, handler
	ldw r2, @(r1, 0)
	ldw r3, @(r1, 4)
	add r1, r0, 0xfffff138
	stw r2, @(r1, 0)
	stw r3, @(r1, 4)
	
	# enable external interrupts - cr0 == PSW
	mvfsys r2, cr0
	or r2, r0, 0x04000000
	mvtsys cr0, r2


	# wait for flag to be set
loop:	
	add r2, r0, flag
	ldw r3, @(r2, 0)
	bratzr r3, loop

	# clear the flag
	stw r0, @(r2, 0)

	add r2, r0, tick
	# putstr
	.long 0x0e000001, 0x00f00000

	bra loop

	# finished
	add r2, r0, r0 || nop
	.long 0x0e000004, 0x00f00000

	
handler:
	jmp real_handler
real_handler:
	add r10, r0, 1
	add r11, r0, flag
	stb r10, @(r11,0)
	reit


flag:	.long 0
tick:	.ascii "Tick\r\n"
