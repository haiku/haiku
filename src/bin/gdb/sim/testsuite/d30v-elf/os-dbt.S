	.globl _start
	#
	# NOTE:	Registers r10-r11 are reserved for the interrupt handler
	#	while the others can be used by the main loop/start code.

_start:		
	# patch the DBT handler
	add r1, r0, handler
	ldw r2, @(r1, 0)
	ldw r3, @(r1, 4)
	# DBT vector address
	add r1, r0, 0xfffff120
	stw r2, @(r1, 0)
	stw r3, @(r1, 4)

	# try out the breakpoint/return
	add r2, r0, 47
	#dbt
	nop
	.long 0x00b00000
	nop

	# exit with what ever the breakpoint hander set r2 to.
	nop
	.long 0x0e000004
	nop

handler:
	jmp real_handler
	
	# The Breakpoint handler sets r2 to 0 if PSW was set correctly.
real_handler:
	mvfsys r2, cr0
	sub r2, r0, 0x08000000
	#rtd
	nop
	.long 0x00a00000
	nop
