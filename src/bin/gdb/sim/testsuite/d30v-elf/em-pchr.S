
	add r2, r0, 'H' 
	.long 0x0e000003, 0x00f00000
	add r2, r0, 'e'
	.long 0x0e000003, 0x00f00000
	add r2, r0, 'l'
	.long 0x0e000003, 0x00f00000
	add r2, r0, 'l'
	.long 0x0e000003, 0x00f00000
	add r2, r0, 'o'
	.long 0x0e000003, 0x00f00000
	add r2, r0, ' '
	.long 0x0e000003, 0x00f00000
	add r2, r0, 'W'
	.long 0x0e000003, 0x00f00000
	add r2, r0, 'o'
	.long 0x0e000003, 0x00f00000
	add r2, r0, 'r'
	.long 0x0e000003, 0x00f00000
	add r2, r0, 'l'
	.long 0x0e000003, 0x00f00000
	add r2, r0, 'd'
	.long 0x0e000003, 0x00f00000
	add r2, r0, '\n' || nop
	.long 0x0e000003, 0x00f00000
	# finished
	add r2, r0, r0 || nop
	.long 0x0e000004, 0x00f00000
