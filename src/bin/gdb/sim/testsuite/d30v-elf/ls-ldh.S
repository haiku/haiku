	# load the 47@addr=60 below into r2
	add r3, r0, 4 || nop
	ldh r2, @(r3,60)
	.long 0x0e000004, 0x00f00000
	.long 0, 0
	.long 0, 0
	.long 0, 0
	.long 0, 0
	.long -1, -1
# address 64 - remember target is BE
	.byte 0, 47, 0xff, 0xff
