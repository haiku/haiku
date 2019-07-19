/*
 * Copyright 2005-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_X86_USER_DEBUGGER_H
#define _KERNEL_ARCH_X86_USER_DEBUGGER_H


#define ARCH_INIT_USER_DEBUG x86_init_user_debug

// number of breakpoints the CPU supports
#define X86_BREAKPOINT_COUNT		4

// debug status register DR6
enum {
	X86_DR6_B0			= 0,	// breakpoint condition detected
	X86_DR6_B1			= 1,	//
	X86_DR6_B2			= 2,	//
	X86_DR6_B3			= 3,	//
	X86_DR6_BD			= 13,	// debug register access detected
	X86_DR6_BS			= 14,	// single step
	X86_DR6_BT			= 15,	// task switch

	X86_DR6_BREAKPOINT_MASK	= (1 << X86_DR6_B0) | (1 << X86_DR6_B1)
								| (1 << X86_DR6_B2) | (1 << X86_DR6_B3),
};

// debug control register DR7 layout:
// 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16
// LEN3  R/W3  LEN2  R/W2  LEN1  R/W1  LEN0  R/W0
//
// 15 14 13 12 11 10 9  8  7  6  5  4  3  2  1  0
// 0  0  GD 0  0  1  GE LE G3 L3 G2 L2 G1 L1 G0 L0
//
enum {
	X86_DR7_L0			= 0,	// local/global breakpoints enable
	X86_DR7_G0			= 1,	//
	X86_DR7_L1			= 2,	//
	X86_DR7_G1			= 3,	//
	X86_DR7_L2			= 4,	//
	X86_DR7_G2			= 5,	//
	X86_DR7_L3			= 6,	//
	X86_DR7_G3			= 7,	//
	X86_DR7_LE			= 8,	// local/global exact breakpoint
	X86_DR7_GE			= 9,	//
	X86_DR7_GD			= 13,	// general detect enable: disallows debug
								// register access
	X86_DR7_RW0_LSB		= 16,	// breakpoints type and len
	X86_DR7_LEN0_LSB	= 18,	//
	X86_DR7_RW1_LSB		= 20,	//
	X86_DR7_LEN1_LSB	= 22,	//
	X86_DR7_RW2_LSB		= 24,	//
	X86_DR7_LEN2_LSB	= 26,	//
	X86_DR7_RW3_LSB		= 28,	//
	X86_DR7_LEN3_LSB	= 30,	//

	X86_BREAKPOINTS_DISABLED_DR7
		= (1 << 10) | (1 << X86_DR7_GE) | (1 << X86_DR7_LE),
		// all breakpoints disabled
};

// the EFLAGS flags we need
enum {
	X86_EFLAGS_CF	= 0,		// carry flag
	X86_EFLAGS_PF	= 2,		// parity flag
	X86_EFLAGS_AF	= 4,		// auxiliary carry flag (adjust flag)
	X86_EFLAGS_ZF	= 6,		// zero flag
	X86_EFLAGS_SF	= 7,		// sign flag
	X86_EFLAGS_TF	= 8,		// trap flag (single stepping)
	X86_EFLAGS_DF	= 10,		// direction flag
	X86_EFLAGS_OF	= 11,		// overflow flag
	X86_EFLAGS_RF	= 16,		// resume flag (skips instruction breakpoint)

	X86_EFLAGS_USER_SETTABLE_FLAGS
		= (1 << X86_EFLAGS_CF) | (1 << X86_EFLAGS_PF) | (1 << X86_EFLAGS_AF)
			| (1 << X86_EFLAGS_ZF) | (1 << X86_EFLAGS_SF) | (1 << X86_EFLAGS_DF)
			| (1 << X86_EFLAGS_OF),
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
	size_t	type;		// one of the architecture types above
	size_t	length;		// one of the length values above
};

struct arch_team_debug_info {
	struct arch_breakpoint	breakpoints[X86_BREAKPOINT_COUNT];

	size_t				dr7;	// debug control register DR7
};

struct arch_thread_debug_info {
	uint32	flags;
};

// The software breakpoint instruction (int3).
extern const uint8 kX86SoftwareBreakpoint[1];

#ifdef __cplusplus
extern "C" {
#endif

struct iframe;

extern void x86_init_user_debug_at_kernel_exit(struct iframe *frame);
extern void x86_exit_user_debug_at_kernel_entry();

extern void x86_handle_debug_exception(struct iframe *frame);
extern void x86_handle_breakpoint_exception(struct iframe *frame);

extern void x86_init_user_debug();

#ifdef __cplusplus
}
#endif

// Feature macros we're supposed to define.
#define DEBUG_MAX_BREAKPOINTS				X86_BREAKPOINT_COUNT
#define DEBUG_MAX_WATCHPOINTS				X86_BREAKPOINT_COUNT
#define DEBUG_SOFTWARE_BREAKPOINT			kX86SoftwareBreakpoint
#define DEBUG_SOFTWARE_BREAKPOINT_SIZE		1
#define DEBUG_SHARED_BREAK_AND_WATCHPOINTS	1


#endif	// _KERNEL_ARCH_X86_USER_DEBUGGER_H
