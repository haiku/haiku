/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ARCH_X86_DEBUGGER_H
#define _ARCH_X86_DEBUGGER_H

struct debug_cpu_state {
	uint8	extended_regs[512];

	uint32	gs;
	uint32	fs;
	uint32	es;
	uint32	ds;
	uint32	edi;
	uint32	esi;
	uint32	ebp;
	uint32	esp;
	uint32	ebx;
	uint32	edx;
	uint32	ecx;
	uint32	eax;
	uint32	vector;
	uint32	error_code;
	uint32	eip;
	uint32	cs;
	uint32	eflags;
	uint32	user_esp;
	uint32	user_ss;
} __attribute__((aligned(8)));

#endif	// _ARCH_X86_DEBUGGER_H
