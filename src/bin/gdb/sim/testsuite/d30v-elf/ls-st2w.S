	add r2, r0, hello
	add r4, r0, ('H' << 24) + ('e' << 16) + ('l' << 8) + ('l' << 0)
	add r5, r0, ('o' << 24) + (' ' << 16) + ('W' << 8) + ('o' << 0)
	st2w r4, @(r2,0) || nop
	# putstr
	.long 0x0e000001, 0x00f00000
	# finished
	add r2, r0, r0 || nop
	.long 0x0e000004, 0x00f00000

	.align 3
hello:  
	.ascii "????....rld\n"
