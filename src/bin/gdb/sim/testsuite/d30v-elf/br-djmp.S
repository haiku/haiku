# perform a delayed jump 47 instructions later
	add r3, r0, insn47
	djmp r3, dest
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
insn47: # actually 45
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
