	# Test macro

	.macro assert reg,value
	cmpeq f0,\reg,\value
	bra/fx fail
	.endm


	# PR 15964 - a.s

	add  r8,r0,0x7fff7fff         ;
	add  r9,r0,0x55555555         ;
	add  r12,r0,0x11111111         ;
	add  r1,r0,0x80000011   ; for psw
	mvtsys psw,r1 ||nop
	addhhhh r12,r8,r9 ||addhlll r13,r12,r12
	mvfsys r20,psw              ||nop
	mvtsys psw,r1 || add r2,r8, r9
	mvfsys r21,psw              ||nop

	assert r20, 0x80000000
	assert r21, 0x80000014

	
	# PR 15964 - b.s
	
	add  r40,r0,0x7fffffff
	add  r41,r0,0x7fffffff
	add  r1,r0,0x80000000       ; for psw
	mvtsys psw,r1,||nop
	cmpeq f1,r40,r41,||cmpeq f0,r40,r41,;
	mvfsys r42,psw
	
	assert r42, 0x80005000

		
	# PR 16993 - a.s

	add  r8,r0,0x80005555  ; for psw
	add  r9,r0,0x80000000  ; for psw
	add  r40,r0,0x11111111  ;
	add  r41,r0,0x22222222  ;
	add  r42,r0,0x00000000  ;
	mvtsys psw,r8 ||nop
	mvtsys psw,r9 ||add r42,r40,r41,;

	mvfsys r10,psw
	assert r10, 0x80000000
	

	# PR 16995 - b.s

	add  r8,r0,0x80000000  ; for psw
	add  r9,r0,0x80005555  ; for psw
	add  r10,r0,0x00000000  ;
	add  r40,r0,0x11111111  ;
	add  r41,r0,0x22222222  ;
	add  r42,r0,0x00000000  ;
	mvtsys psw,r8 ||nop
	mvtsys psw,r9 ||add r42,r40,r41,;

	mvfsys r10,psw
	assert r10, 0x80005544
	

	# PR 17006 - c.s

	add  r8,r0,0x80005555  ; for psw
	add  r9,r0,0x80000000  ; for psw
	add  r10,r0,0x00000000  ;
	add  r40,r0,0x00000011  ;
	add  r41,r0,0x00000011  ;
	mvtsys psw,r8 ||nop
	mvtsys psw,r9 ||cmpeq f5,r40,r41,;
		
	mvfsys r10,psw
	assert r10, 0x80000010


	# PR 17006 - d.s

	add  r9,r0,0x80000000  ; for psw
	add  r40,r0,0x00000011  ;
	add  r41,r0,0x00000011  ;
	nop  ||nop
	mvtsys psw, r9 || nop
	nop  ||nop
	nop ||cmpeq f5,r40,r41,;

	mvfsys r10,psw
	assert r10, 0x80000010

	
	# PR 17106 - a.s

	; test 000     ;  mvtsys(s=0) || sathl(s=0) prallel execution test
	add  r8,r0,0x80005555  ; for psw
	add  r9,r0,0x80000000  ; for psw
	add  r40,r0,0x00000044  ;
	add  r41,r0,0x00000008  ;
	mvtsys psw,r8 ||nop
	mvtsys psw,r9 ||sathl r30,r40,r41,;
	mvfsys r20, psw ||nop
	;-------------------------------
	; test 001     ;  mvtsys(s=0) || sathl(s=1) prallel execution test
	_test_001:
	add  r40,r0,0x00004444  ;
	add  r41,r0,0x00000008  ;
	mvtsys psw,r8 ||nop
	mvtsys psw,r9 ||sathl r31,r40,r41,;
	mvfsys r21,psw ||nop
	;-------------------------------
	; test 002     ;  mvtsys(s=1) || sathl(s=0) prallel execution test
	add  r8,r0,0x80000000  ; for psw
	add  r9,r0,0x80005555  ; for psw
	add  r40,r0,0x00000044  ;
	add  r41,r0,0x00000008  ;
	mvtsys psw,r8 ||nop
	mvtsys psw,r9 ||sathl r32,r40,r41,;
	mvfsys r22,psw             ||nop
	;-------------------------------
	; test 003     ;  mvtsys(s=1) || sathl(s=1) prallel execution test
	;   init-reg
	add  r40,r0,0x00004444  ;
	add  r41,r0,0x00000008  ;
	mvtsys psw,r8 ||nop
	mvtsys psw,r9 ||sathl r33,r40,r41,;
	mvfsys r23,psw             ||nop

	assert r20, 0x80000000
	assert r21, 0x80000040
	assert r22, 0x80005555
	assert r23, 0x80005515

	
	# PR 18288 - a.s

	;------------------------------------------------------------------------
	; mvtsys (C =1, V= VA = 0) ||  addc (C= V= VA =0)
	;------------------------------------------------------------------------
	test_000b:
	add r1,r0,1   ||nop  ; set C bit
	mvtsys psw r0 ||nop
	mvtsys psw r1 ||addc r20,r0,1 
	mvfsys r10,psw ||nop
	; C changed in MU is not used in IU.
	; IU prevail for resulting C. 
	;------------------------------------------------------------------------
	; mvtsys (V =1, C = VA = 0) ||  add (C= V= VA =0)
	;------------------------------------------------------------------------
	test_001b:
	add r1,r0,0x10 ||nop  ; set V bit
	mvtsys psw r0 ||nop
	mvtsys psw r1 ||add r0,r0,r0
	mvfsys r11,psw ||nop
	; IU prevail for resulting V.
	;------------------------------------------------------------------------
	; mvtsys (V = C= VA = 0) ||  add (C=0,V= VA =1)
	;------------------------------------------------------------------------
	test_002b:
	add r1,r0,0x70000000
	add r2,r0,0x30000000
	mvtsys psw r0 ||nop
	mvtsys psw r0 ||add r0,r1,r2
	mvfsys r12,psw ||nop
	; IU prevail for resulting V. 
	; VA is set(OR'ed)
	;------------------------------------------------------------------------
	; mvtsys (C= 0 V = VA = 1) ||  add (C= V= VA =0)
	;------------------------------------------------------------------------
	test_003b:
	add r1,r0,0x14 ||nop  ; set V and VA bit
	mvtsys psw r0 ||nop
	mvtsys psw r1 ||add r0,r0,r0
	mvfsys r13,psw ||nop
	; IU prevail for resulging V
	; VA is set(OR'ed)
	;------------------------------------------------------------------------
	; mvtsys (f3 =1) ||  orfg (f3) : GROUP_B
	;------------------------------------------------------------------------
	test_004b:
	add r1,r0,0x100 ; set f3
	mvtsys psw r0 ||nop
	mvtsys psw,r1 ||orfg f3,f3,0
	mvfsys r14,psw ||nop
	; results of IU prevail. 
	;------------------------------------------------------------------------
	; mvtsys (f4 =1) ||  sathp 
	;------------------------------------------------------------------------
	test_005b:
	add r1,r0,0x40 ; set f4
	mvtsys psw r0 ||nop
	mvtsys psw r1 ||sathl r2,r1,3
	mvfsys r15,psw ||nop
	; results of MU is used in IU

	assert r20, 0x1
	assert r10, 0x0
	assert r11, 0x0
	assert r12, 0x14
	assert r13, 0x4
	assert r14, 0x0
	assert r15, 0x0

	
	# PR 18288 - b.s
	add r7,r0,0x80000000
	mvtsys psw,r7 || nop
		
	add r8,r0,0x7fff7fff         ;
	add r9,r0,0x55555555         ;
	add r12,r0,0x11111111         ;
	add r13,r0,0x00000000         ;
	addhhhh r12,r8,r9 ||addhlll r13,r12,r12
	mvfsys r60,psw              ||nop
	;------------------------------------------
	add r20,r0,0x66666666  ;
	add r21,r0,0x77777777  ;
	add r40,r0,0x22222222  ;
	add r41,r0,0x55555555  ;
	add r22,r20,r21 ||add r42,r40,r41,;
	mvfsys r61,psw              ||nop

	assert r60, 0x80000000
	assert r61, 0x80000000
	

	# PR 19224

	add r7,r0,0x80000000
	add r2,r0,r0            || nop
	add r1,r0,0x1           || nop
	# confirm that these insns do not kill the add in the right container
	mvtsys psw,r7           -> add r2,r2,r1
	mvtsys pswh,r7          -> add r2,r2,r1
	mvtsys pswl,r7          -> add r2,r2,r1	
	mvtsys f0,r7            -> add r2,r2,r1			
	mvtsys mod_s,r7         -> add r2,r2,r1							

	assert r2, 0x5

									
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
