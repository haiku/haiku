	# compute -2(from mem) + 49 = 47
	add r3, r0, 4 || nop
	ldb r2, @(r3,60)
	add r2, r2, 49
	.long 0x0e000004, 0x00f00000
	.long 0, 0
	.long 0, 0
	.long 0, 0
	.long -1, -1
# address 64 - remember target is BE
	.byte -2, 0xff, 0xff, 0xff
