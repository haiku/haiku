	# compute 254(mem) - 207 = 47
	add r3, r0, 4 || nop
	ldbu r2, @(r3,60)
	sub r2, r2, 207
	.long 0x0e000004, 0x00f00000
	.long 0, 0
	.long 0, 0
	.long 0, 0
	.long -1, -1
# address 64 - remember target is BE
	.byte -2, 0xff, 0xff, 0xff
