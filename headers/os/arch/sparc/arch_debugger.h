/*
 * Copyright 2005-2019, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ARCH_SPARC_DEBUGGER_H
#define _ARCH_SPARC_DEBUGGER_H


struct sparc_debug_cpu_state {
	uint64	pc;
	uint64 i6; // frame pointer
} __attribute__((aligned(8)));


#endif	// _ARCH_SPARC_DEBUGGER_H

