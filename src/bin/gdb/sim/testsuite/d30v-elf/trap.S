# verify that trap || cmp works
	add  r8,r0,0x11223344         ;
	add  r9,r0,0x11223344         ;

	mvtsys bpsw,r0 || nop
	mvtsys bpc,r0  || nop

	add  r1,r0,0x97000555       ; for psw
	mvtsys psw,r1 || nop
	trap 0  || cmpeq f0,r8,r9,;

	.long 0x0e000004, 0x00f00000

	.section .eit_v, "a"
	nop     || nop
	nop     || nop
	nop     || nop
	nop     || nop

# save the old bpsw, psw
	mvfsys r4,bpsw || nop
	mvfsys r5,psw || nop

# load up what they should be
	add r6,r0,0x97004555
	add r7,r0,0x90000000

# verify that they have the right values
# return exit value in r2 -- 0 success, 47 failure
	add r2,r0,47
	cmpeq f0,r4,r6 || nop
	cmpeq f1,r5,r7 || nop
	add/tt r2,r0,r0 || nop

	reit
