	
	add r2, r0, hello
	# putstr
	.long 0x0e000001, 0x00f00000
	# finished
	add r2, r0, r0 || nop
	.long 0x0e000004, 0x00f00000

hello:	.ascii "Hello World\r\n"
