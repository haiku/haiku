	add r2, r0, hello
	add r4, r0, ('H' << 8) + ('e' << 0)
	sth r4, @(r2,0) || nop
	# putstr
	.long 0x0e000001, 0x00f00000
	# finished
	add r2, r0, r0 || nop
	.long 0x0e000004, 0x00f00000

	.align 3
hello:  
	.ascii "??llo World\n"
