	# Test macro

	.macro assert reg,value
	cmpeq f0,\reg,\value
	bra/fx fail
	.endm

	.macro nassert reg,value
	cmpne f0,\reg,\value
	bra/fx fail
	.endm

	
	# PR 18452 - a.s

	add r1, r0, 0
	add r2,r0,0x11223344
	add r3,r0,0x8899aabb
	nop   ||mulx2h r0,r2,r3

	assert r0, 0x0
	assert r1, 0x0

	# test other double-word loads

	add	r2,r0,data
	ld2w	r0,@(r2,0)
	assert	r0, 0x0
	assert	r1, 0x0

	add	r2,r0,data
	ld2h	r0,@(r2,0)
	assert	r0, 0x0
	assert	r1, 0x0

	add	r2,r0,data
	ld4bh	r0,@(r2,0)
	assert	r0, 0x0
	assert	r1, 0x0

	add	r2,r0,data
	ld4bhu	r0,@(r2,0)
	assert	r0, 0x0
	assert	r1, 0x0

	
	# PR 18679 - a.s

	ld2h	r2, @(r0, b)
	ld2h	r4, @(r0, d)
	assert	r2, 0x00001111
	assert	r3, 0xffff8899
	assert	r4, 0x00001111
	assert	r5, 0x00002222
	
					
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

	# some non-zero data
data:
	.long 0x12345678
	.long 0x9abcdef0
	.long 0xdeadbeef

b:
	.word 0x11118899, 0x0
d:
	.word 0x11112222, 0x0
