# perform a delayed branch 47 instructions later
	add r3, r0, 47
	srl r3, r3, -3
	dbra r3, dest
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop

.align 4
dest:	
	.long 0x0e000004, 0x00f00000
