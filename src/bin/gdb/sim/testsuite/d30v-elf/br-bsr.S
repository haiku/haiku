	# verify that the return address is set correctly
	add r2, r0, 1
	add r3, r0, dest1
	sub r3, r3, off1
off1:
	bsr r3 || nop
ret1:

	.align 5
dest1:	
	sub r2, r62, ret1
	.long 0x0e000004, 0x00f00000
