/*
 * Copyright 2005-2009, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ARCH_X86_DEBUGGER_H
#define _ARCH_X86_DEBUGGER_H


typedef struct x86_fp_register {
	uint8		value[10];
	uint8		reserved[6];
} x86_fp_register;


typedef struct x86_xmm_register {
	uint8		value[16];
} x86_xmm_register;


typedef struct x86_extended_registers {
	uint16				control;
	uint16				status;
	uint8				tag;
	uint8				reserved1;
	uint16				opcode;
	uint32				instruction_pointer;
	uint16				cs;
	uint16				reserved2;
	uint32				data_pointer;
	uint16				ds;
	uint16				reserved3;
	uint32				mxcsr;
	uint32				mxcsr_mask;
	union {
		x86_fp_register	fp_registers[8];	// st0-st7
		x86_fp_register	mmx_registers[8];	// mm0-mm7
	};
	x86_xmm_register	xmm_registers[8];	// xmm0-xmm7
	uint8				reserved4[224];		// 288 - 512
} x86_extended_registers;


struct x86_debug_cpu_state {
	x86_extended_registers	extended_registers;

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
} __attribute__((aligned(16)));


#endif	// _ARCH_X86_DEBUGGER_H
