	# compute (47 || 0|16) >> 16
	add r3, r0, 4 || nop
	ldhh r2, @(r3,60)
	srl r2, r2, 16 || nop
	.long 0x0e000004, 0x00f00000
	.long 0, 0
	.long 0, 0
	.long 0, 0
	.long -1, -1
# address 64 - remember target is BE
	.byte 0, 47, 0xff, 0xff
