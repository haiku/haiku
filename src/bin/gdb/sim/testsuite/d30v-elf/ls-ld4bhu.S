	# compute lo : 48(mem+1) + 255(mem+3) - 256 = 47
	add r3, r0, 4 || nop
	ld4bhu r2, @(r3,60)
	add r2, r2, r3 || nop
	sub r2, r2, 0x100
	.long 0x0e000004, 0x00f00000
	.long 0, 0
	.long 0, 0
	.long -1, -1
# address 64 - remember target is BE
	.byte 0, 48, 0, 255
