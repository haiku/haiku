/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMPAT_ARCH_SIGNAL_H
#define _KERNEL_COMPAT_ARCH_SIGNAL_H


typedef struct packed_fp_stack {
	unsigned char	st0[10];
	unsigned char	st1[10];
	unsigned char	st2[10];
	unsigned char	st3[10];
	unsigned char	st4[10];
	unsigned char	st5[10];
	unsigned char	st6[10];
	unsigned char	st7[10];
} packed_fp_stack;

typedef struct packed_mmx_regs {
	unsigned char	mm0[10];
	unsigned char	mm1[10];
	unsigned char	mm2[10];
	unsigned char	mm3[10];
	unsigned char	mm4[10];
	unsigned char	mm5[10];
	unsigned char	mm6[10];
	unsigned char	mm7[10];
} packed_mmx_regs;

typedef struct xmmx_regs {
	unsigned char	xmm0[16];
	unsigned char	xmm1[16];
	unsigned char	xmm2[16];
	unsigned char	xmm3[16];
	unsigned char	xmm4[16];
	unsigned char	xmm5[16];
	unsigned char	xmm6[16];
	unsigned char	xmm7[16];
} xmmx_regs;

typedef struct compat_stack_t {
	uint32	ss_sp;
	uint32	ss_size;
	int		ss_flags;
} compat_stack_t;

typedef struct compat_old_extended_regs {
	unsigned short	fp_control;
	unsigned short	_reserved1;
	unsigned short	fp_status;
	unsigned short	_reserved2;
	unsigned short	fp_tag;
	unsigned short	_reserved3;
	uint32	fp_eip;
	unsigned short	fp_cs;
	unsigned short	fp_opcode;
	uint32	fp_datap;
	unsigned short	fp_ds;
	unsigned short	_reserved4;
	union {
		packed_fp_stack	fp;
		packed_mmx_regs	mmx;
	} fp_mmx;
} compat_old_extended_regs;

typedef struct compat_new_extended_regs {
	unsigned short	fp_control;
	unsigned short	fp_status;
	unsigned short	fp_tag;
	unsigned short	fp_opcode;
	uint32	fp_eip;
	unsigned short	fp_cs;
	unsigned short	res_14_15;
	uint32	fp_datap;
	unsigned short	fp_ds;
	unsigned short	_reserved_22_23;
	uint32	mxcsr;
	uint32	_reserved_28_31;
	union {
		fp_stack fp;
		mmx_regs mmx;
	} fp_mmx;
	xmmx_regs xmmx;
	unsigned char	_reserved_288_511[224];
} compat_new_extended_regs;

typedef struct compat_extended_regs {
	union {
		compat_old_extended_regs	old_format;
		compat_new_extended_regs	new_format;
	} state;
	uint32	format;
} compat_extended_regs;

struct compat_vregs {
	uint32			eip;
	uint32			eflags;
	uint32			eax;
	uint32			ecx;
	uint32			edx;
	uint32			esp;
	uint32			ebp;
	uint32			_reserved_1;
	compat_extended_regs	xregs;
	uint32			edi;
	uint32			esi;
	uint32			ebx;
};


#endif // _KERNEL_COMPAT_ARCH_SIGNAL_H
