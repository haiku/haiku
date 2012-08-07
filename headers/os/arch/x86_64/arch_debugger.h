/*
 * Copyright 2005-2012, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ARCH_X86_64_DEBUGGER_H
#define _ARCH_X86_64_DEBUGGER_H


typedef struct x86_64_fp_register {
	uint8		value[10];
	uint8		reserved[6];
} x86_64_fp_register;


typedef struct x86_64_xmm_register {
	uint8		value[16];
} x86_64_xmm_register;


typedef struct x86_64_extended_registers {
	uint16					control;
	uint16					status;
	uint8					tag;
	uint8					reserved1;
	uint16					opcode;
	uint64					instruction_pointer;
	uint64					data_pointer;
	uint32					mxcsr;
	uint32					mxcsr_mask;
	union {
		x86_64_fp_register	fp_registers[8];	// st0-st7
		x86_64_fp_register	mmx_registers[8];	// mm0-mm7
	};
	x86_64_xmm_register		xmm_registers[16];	// xmm0-xmm15
	uint8					reserved2[96];		// 416 - 512
} x86_64_extended_registers;


struct x86_64_debug_cpu_state {
	x86_64_extended_registers	extended_registers;

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
	uint64	rip;
	uint64	cs;
	uint64	rflags;
	uint64	rsp;
	uint64	ss;
} __attribute__((aligned(16)));


#endif	// _ARCH_X86_64_DEBUGGER_H
