/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_X86_USER_DEBUGGER_H
#define _KERNEL_ARCH_X86_USER_DEBUGGER_H

// number of breakpoints the CPU supports
enum {
	X86_BREAKPOINT_COUNT = 4,
};

// debug control register DR7 layout:
// 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16
// LEN3  R/W3  LEN2  R/W2  LEN1  R/W1  LEN0  R/W0
//
// 15 14 13 12 11 10 9  8  7  6  5  4  3  2  1  0
// 0  0  GD 0  0  1  GE LE G3 L3 G2 L2 G1 L1 G0 L0
//
enum {
	X86_DR7_LEN3_LSB	= 30,	// breakpoints len and type
	X86_DR7_RW3_LSB	= 28,		//
	X86_DR7_LEN2_LSB	= 26,	//
	X86_DR7_RW2_LSB	= 24,		//
	X86_DR7_LEN1_LSB	= 22,	//
	X86_DR7_RW1_LSB	= 20,		//
	X86_DR7_LEN0_LSB	= 18,	//
	X86_DR7_RW0_LSB	= 16,		//
	X86_DR7_GD		= 13,		// general detect enable: disallows debug
								// register access
	X86_DR7_GE		= 9,		// global/local exact breakpoint
	X86_DR7_LE		= 8,		//
	X86_DR7_G3		= 7,		// global/local breakpoints enable
	X86_DR7_L3		= 6,		//
	X86_DR7_G2		= 5,		//
	X86_DR7_L2		= 4,		//
	X86_DR7_G1		= 3,		//
	X86_DR7_L1		= 2,		//
	X86_DR7_G0		= 1,		//
	X86_DR7_L0		= 0,		//

	X86_BREAKPOINTS_DISABLED_DR7
		= (1 << 10) | (1 << X86_DR7_GE) | (1 << X86_DR7_LE),
		// all breakpoints disabled
};

// x86 breakpoint types
enum {
	X86_INSTRUCTION_BREAKPOINT		= 0x0,
	X86_DATA_WRITE_BREAKPOINT		= 0x1,
	X86_IO_READ_WRITE_BREAKPOINT	= 0x2,		// >= 586
	X86_DATA_READ_WRITE_BREAKPOINT	= 0x3,
};

// x86 breakpoint lengths
enum {
	X86_BREAKPOINT_LENGTH_1	= 0x0,
	X86_BREAKPOINT_LENGTH_2	= 0x1,
	X86_BREAKPOINT_LENGTH_4	= 0x3,
};

struct arch_breakpoint {
	void	*address;	// NULL, if deactivated
	uint32	type;		// one of the architecture types above
	uint32	length;		// one of the length values above
};

struct arch_team_debug_info {
	struct arch_breakpoint	breakpoints[X86_BREAKPOINT_COUNT];

	uint32					dr7;	// debug control register DR7
};

#endif	// _KERNEL_ARCH_X86_USER_DEBUGGER_H
