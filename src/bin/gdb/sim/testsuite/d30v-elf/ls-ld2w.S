	# compute 17(mem) + 30(mem+4) = 47
	add r3, r0, 4 || nop
	ld2w r2, @(r3,60)
	add r2, r2, r3 || nop
	.long 0x0e000004, 0x00f00000
	.long 0, 0
	.long 0, 0
	.long 0, 0
	.long -1, -1
# address 64 - remember target is BE
	.byte 0, 0, 0, 17
	.byte 0, 0, 0, 30
