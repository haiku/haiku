# Verify that the exit call works
	add r2, r0, 0 || nop
	.long 0x0e000004
	nop
