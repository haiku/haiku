/*
 * Copyright 2005-2012, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ARCH_X86_64_DEBUGGER_H
#define _ARCH_X86_64_DEBUGGER_H


#include <posix/arch/x86_64/signal.h>


struct x86_64_debug_cpu_state {
	struct savefpu	extended_registers;

	uint64	gs;
	uint64	fs;
	uint64	es;
	uint64	ds;
	uint64	r15;
	uint64	r14;
	uint64	r13;
	uint64	r12;
	uint64	r11;
	uint64	r10;
	uint64	r9;
	uint64	r8;
	uint64	rbp;
	uint64	rsi;
	uint64	rdi;
	uint64	rdx;
	uint64	rcx;
	uint64	rbx;
	uint64	rax;
	uint64	vector;
	uint64	error_code;
	uint64	rip;
	uint64	cs;
	uint64	rflags;
	uint64	rsp;
	uint64	ss;
} __attribute__((aligned(16)));


#endif	// _ARCH_X86_64_DEBUGGER_H
