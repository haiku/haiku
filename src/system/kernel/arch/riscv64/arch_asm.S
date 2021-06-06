/*
 * Copyright 2019, Haiku, Inc. All Rights Reserved
 * Distributed under the terms of the MIT License.
 */


#include <asm_defs.h>


.text


FUNCTION(MSyscall):
	ecall
	ret
FUNCTION_END(MSyscall)


FUNCTION(arch_setjmp):
	sd ra,   0*8(a0)
	sd s0,   1*8(a0)
	sd s1,   2*8(a0)
	sd s2,   3*8(a0)
	sd s3,   4*8(a0)
	sd s4,   5*8(a0)
	sd s5,   6*8(a0)
	sd s6,   7*8(a0)
	sd s7,   8*8(a0)
	sd s8,   9*8(a0)
	sd s9,  10*8(a0)
	sd s10, 11*8(a0)
	sd s11, 12*8(a0)
	sd sp,  13*8(a0)
	csrr t0, satp
	sd t0,  14*8(a0)

	li    a0, 0
	ret
FUNCTION_END(arch_setjmp)


FUNCTION(arch_longjmp):
	ld ra,   0*8(a0)
	ld s0,   1*8(a0)
	ld s1,   2*8(a0)
	ld s2,   3*8(a0)
	ld s3,   4*8(a0)
	ld s4,   5*8(a0)
	ld s5,   6*8(a0)
	ld s6,   7*8(a0)
	ld s7,   8*8(a0)
	ld s8,   9*8(a0)
	ld s9,  10*8(a0)
	ld s10, 11*8(a0)
	ld s11, 12*8(a0)
	ld sp,  13*8(a0)
	ld t0,  14*8(a0)
	csrw satp, t0
	sfence.vma

	seqz a0, a1
	add  a0, a0, a1   # a0 = (a1 == 0) ? 1 : a1
	ret
FUNCTION_END(arch_longjmp)


FUNCTION(save_fpu):
	fsd f0,   0*8(a0)
	fsd f1,   1*8(a0)
	fsd f2,   2*8(a0)
	fsd f3,   3*8(a0)
	fsd f4,   4*8(a0)
	fsd f5,   5*8(a0)
	fsd f6,   6*8(a0)
	fsd f7,   7*8(a0)
	fsd f8,   8*8(a0)
	fsd f9,   9*8(a0)
	fsd f10, 10*8(a0)
	fsd f11, 11*8(a0)
	fsd f12, 12*8(a0)
	fsd f13, 13*8(a0)
	fsd f14, 14*8(a0)
	fsd f15, 15*8(a0)
	fsd f16, 16*8(a0)
	fsd f17, 17*8(a0)
	fsd f18, 18*8(a0)
	fsd f19, 19*8(a0)
	fsd f20, 20*8(a0)
	fsd f21, 21*8(a0)
	fsd f22, 22*8(a0)
	fsd f23, 23*8(a0)
	fsd f24, 24*8(a0)
	fsd f25, 25*8(a0)
	fsd f26, 26*8(a0)
	fsd f27, 27*8(a0)
	fsd f28, 28*8(a0)
	fsd f29, 29*8(a0)
	fsd f30, 30*8(a0)
	fsd f31, 31*8(a0)
	frcsr t0
	sd  t0,  32*8(a0)

	ret
FUNCTION_END(save_fpu)


FUNCTION(restore_fpu):
	fld f0,   0*8(a0)
	fld f1,   1*8(a0)
	fld f2,   2*8(a0)
	fld f3,   3*8(a0)
	fld f4,   4*8(a0)
	fld f5,   5*8(a0)
	fld f6,   6*8(a0)
	fld f7,   7*8(a0)
	fld f8,   8*8(a0)
	fld f9,   9*8(a0)
	fld f10, 10*8(a0)
	fld f11, 11*8(a0)
	fld f12, 12*8(a0)
	fld f13, 13*8(a0)
	fld f14, 14*8(a0)
	fld f15, 15*8(a0)
	fld f16, 16*8(a0)
	fld f17, 17*8(a0)
	fld f18, 18*8(a0)
	fld f19, 19*8(a0)
	fld f20, 20*8(a0)
	fld f21, 21*8(a0)
	fld f22, 22*8(a0)
	fld f23, 23*8(a0)
	fld f24, 24*8(a0)
	fld f25, 25*8(a0)
	fld f26, 26*8(a0)
	fld f27, 27*8(a0)
	fld f28, 28*8(a0)
	fld f29, 29*8(a0)
	fld f30, 30*8(a0)
	fld f31, 31*8(a0)
	ld  t0,  32*8(a0)
	fscsr t0

	ret
FUNCTION_END(restore_fpu)


FUNCTION(arch_thread_entry):
	mv a0, s2
	jalr s1
FUNCTION_END(arch_thread_entry)


FUNCTION(arch_enter_userspace):
	mv sp, a2
	sret
FUNCTION_END(arch_enter_userspace)


FUNCTION(arch_longjmp_iframe):
	mv sp, a0
	call SVecURet
FUNCTION_END(arch_longjmp_iframe)
