        # Test macro

        .macro assert reg,value
        cmpeq f0,\reg,\value
        bra/fx fail
        .endm

	
	# PR 18230
	
	add  r62,r0,0xffffffff || nop
	add  r1,r0,24 || nop
	djsr r1,test_end
	add  r2,r0,r62
test_end:	nop
	
	assert r2, 0x00000030
	assert r62, 0x00000030

			
# check return address correctly set by a djsr insn

	add r3, r0, last
	sub r3, r3, start
start:	
	djsr r3, dest
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
last:
	add r2, r2, 1 || nop
nexti:	
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop
	add r2, r2, 1 || nop

.align 4
dest:

	assert r62,nexti



	# all okay
	bra ok
		
ok:
        add r2, r0, 0
        .long 0x0e000004
        nop

fail:
        add r2, r0, 47
        .long 0x0e000004
        nop
