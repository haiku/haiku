	add r3, r0, dest1
	sub r3, r3, off1
off1:
	bratzr r2, r3

	.align 4
dest1:	
	bratzr r3, dest2
	.long 0x0e000004, 0x00f00000

	.align 5
dest2:	
