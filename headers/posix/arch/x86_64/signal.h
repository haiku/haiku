/*
 * Copyright 2002-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ARCH_SIGNAL_H_
#define _ARCH_SIGNAL_H_


/*
 * Architecture-specific structure passed to signal handlers
 */

#if __x86_64__


struct fp_stack {
	unsigned char		st0[10];
	unsigned char		_reserved_42_47[6];
	unsigned char		st1[10];
	unsigned char		_reserved_58_63[6];
	unsigned char		st2[10];
	unsigned char		_reserved_74_79[6];
	unsigned char		st3[10];
	unsigned char		_reserved_90_95[6];
	unsigned char		st4[10];
	unsigned char		_reserved_106_111[6];
	unsigned char		st5[10];
	unsigned char		_reserved_122_127[6];
	unsigned char		st6[10];
	unsigned char		_reserved_138_143[6];
	unsigned char		st7[10];
	unsigned char		_reserved_154_159[6];
};

struct mmx_regs {
	unsigned char		mm0[10];
	unsigned char		_reserved_42_47[6];
	unsigned char		mm1[10];
	unsigned char		_reserved_58_63[6];
	unsigned char		mm2[10];
	unsigned char		_reserved_74_79[6];
	unsigned char		mm3[10];
	unsigned char		_reserved_90_95[6];
	unsigned char		mm4[10];
	unsigned char		_reserved_106_111[6];
	unsigned char		mm5[10];
	unsigned char		_reserved_122_127[6];
	unsigned char		mm6[10];
	unsigned char		_reserved_138_143[6];
	unsigned char		mm7[10];
	unsigned char		_reserved_154_159[6];
};

struct xmm_regs {
	unsigned char		xmm0[16];
	unsigned char		xmm1[16];
	unsigned char		xmm2[16];
	unsigned char		xmm3[16];
	unsigned char		xmm4[16];
	unsigned char		xmm5[16];
	unsigned char		xmm6[16];
	unsigned char		xmm7[16];
	unsigned char		xmm8[16];
	unsigned char		xmm9[16];
	unsigned char		xmm10[16];
	unsigned char		xmm11[16];
	unsigned char		xmm12[16];
	unsigned char		xmm13[16];
	unsigned char		xmm14[16];
	unsigned char		xmm15[16];
};

struct fpu_state {
	unsigned short		control;
	unsigned short		status;
	unsigned short		tag;
	unsigned short		opcode;
	unsigned long		rip;
	unsigned long		rdp;
	unsigned int		mxcsr;
	unsigned int		mscsr_mask;

	union {
		struct fp_stack	fp;
		struct mmx_regs	mmx;
	};

	struct xmm_regs		xmm;
	unsigned char		_reserved_416_511[96];
};

struct vregs {
	unsigned long		rax;
	unsigned long		rbx;
	unsigned long		rcx;
	unsigned long		rdx;
	unsigned long		rdi;
	unsigned long		rsi;
	unsigned long		rbp;
	unsigned long		r8;
	unsigned long		r9;
	unsigned long		r10;
	unsigned long		r11;
	unsigned long		r12;
	unsigned long		r13;
	unsigned long		r14;
	unsigned long		r15;

	unsigned long		rsp;
	unsigned long		rip;
	unsigned long		rflags;

	struct fpu_state	fpu;
};


#endif /* __x86_64__ */

#endif /* _ARCH_SIGNAL_H_ */
