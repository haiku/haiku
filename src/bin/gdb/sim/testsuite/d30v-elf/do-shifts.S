	# Test macro

	.macro assert reg,value
	cmpeq f0,\reg,\value
	bra/fx fail
	.endm


	# PR 14580 - a.s

	add  r8,r0,0x11112222
	add  r9,r0,-32
	sra r1,r8,r9            ||nop
	sra r2,r8,-32           ||nop
	srl r3,r8,r9            ||nop
	srl r4,r8,-32           ||nop

	assert r1, 0
	assert r2, 0
	assert r3, 0
	assert r4, 0


	# PR 17266 - a.s

	add r20, r0, 0xffffffff
	add r21, r0, 0xffffffff
	add r22, r0, 0xffffffff
	add r23, r0, 0xffffffff
	add r1, r0, 0x12345678 
	add r2, r0, -33
	srahh r20, r1, r2
	srahl r21, r1, r2
	srlhh r22, r1, r2
	srlhl r23, r1, r2
	sra r24, r1, r2
	srl r25, r1, r2
	rot r26, r1, r2

	assert r20, 0xacf0ffff
	assert r21, 0xffffacf0
	assert r22, 0xacf0ffff
	assert r23, 0xffffacf0
	assert r24, 0x2468acf0
	assert r25, 0x2468acf0
	assert r26, 0x2468acf0

	
	# PR 17266 - a2.s

	add r20, r0, 0xffffffff
	add r21, r0, 0xffffffff
	add r22, r0, 0xffffffff
	add r23, r0, 0xffffffff
	add r1, r0, 0x12345678 
	add r2, r0, -17
	sra2h r20, r1, r2
	srl2h r21, r1, r2
	rot2h r22, r1, r2

	assert r20, 0x2468acf0
	assert r21, 0x2468acf0
	assert r22, 0x2468acf0


	# PR 17685 - a.s

	add r20,r0,r0
	add r21,r0,r0
	add r22,r0,r0
	add r23,r0,r0
	add r24,r0,r0
	add r25,r0,r0
	add r30,r0,r0
	add r31,r0,r0

	add  r8,r0,0x55555555
	add  r9,r0,0x1f
	sra r20,r8,r9 ||nop
	srl r21,r8,r9 ||nop
	srahh r22,r8,r9 ||nop
	srahl r23,r8,r9 ||nop
	srlhh r24,r8,r9 ||nop
	srlhl r25,r8,r9 ||nop

	add  r8,r0,0x5555aaaa
	add  r9,r0,0x000ffff1
	sra2h r30,r8,r9 ||nop
	srl2h r31,r8,r9 ||nop

	assert	r20, 0
	assert	r21, 0
	assert	r22, 0
	assert	r23, 0
	assert	r24, 0
	assert	r25, 0
	assert	r30, 0
	assert	r31, 0

	
	# PR 18196 - a.s

	add r1,r0,0xfedcba98
	add r2,r0,0x76543210
	add r3,r0,0x41           
	add r4,r0,1           
	nop                      || mvtacc a0 r1,r2
	nop                      || mvfacc r10,a0 r3      
	nop                      || mvfacc r11,a0 r4      

	assert r10, 0x3b2a1908
	assert r11, 0x3b2a1908

	
	# PR 18329 - a.s

	add r10,r0,0  || add r2,r0,-1
	add r11,r0,0  || add r12,r0,0
	add r13,r0,0  || add r14,r0,0
	add r15,r0,0  || add r16,r0,0
	add r17,r0,0  || add r18,r0,0
	
	add r3,r0,0x00000020    ; 32
	src r10,r2,r3
	add r3,r0,0x00000021    ; 33
	src r11,r2,r3
	add r3,r0,0x0000003f    ; 63
	src r12,r2,r3
	add r3,r0,0x00000040    ;64
	src r13,r2,r3
	add r3,r0,0x00000041    ;65
	src r14,r2,r3
	
	add r3,r0,0xffffffdf    ;-33
	src r15,r2,r3
	add r3,r0,0xffffffc1    ;-63
	src r16,r2,r3
	add r3,r0,0xffffffc0    ;-64
	src r17,r2,r3
	add r3,r0,0xffffffbf    ;-65
	src r18,r2,r3
	
	assert r10, 0x00000000
	assert r11, 0x80000000
	assert r12, 0xfffffffe
	assert r13, 0x00000000
	assert r14, 0x80000000
	assert r15, 0x00000001
	assert r16, 0x7fffffff
	assert r17, 0xffffffff
	assert r18, 0x00000001


	# PR 18364 - b.s

	add r1,r0,0x12345678
	add r2,r0,0x9abcdef0
	add r3,r0,0xffffffe0 ; -32
	add r4,r0,0xffffffc0 ; -64
	src r1,r2,r3 ||nop
	add r10,r1,r0 ||nop
	add r1,r0,0x12345678
	src r1,r2,r4 ||nop
	add r11,r1,r0 ||nop

	assert r10, 0x9abcdef0
	assert r11, 0x9abcdef0

				
	# all okay

	bra	ok

ok:
	add r2, r0, 0
	.long 0x0e000004
	nop

fail:
	add r2, r0, 47
	.long 0x0e000004
	nop
