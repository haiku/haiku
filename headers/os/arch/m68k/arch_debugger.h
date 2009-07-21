/*
 * Copyright 2005-2009, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ARCH_M68K_DEBUGGER_H
#define _ARCH_M68K_DEBUGGER_H

struct m68k_debug_cpu_state {
	uint32	d0;
	uint32	d1;
	uint32	d2;
	uint32	d3;
	uint32	d4;
	uint32	d5;
	uint32	d6;
	uint32	d7;
	uint32	a0;
	uint32	a1;
	uint32	a2;
	uint32	a3;
	uint32	a4;
	uint32	a5;
	uint32	a6;
	uint32	a7;
	uint32	pc;
	uint16	sr;
//#warning M68K: missing members!
	uint32	dummy;
} __attribute__((aligned(8)));

#endif	// _ARCH_M68K_DEBUGGER_H
