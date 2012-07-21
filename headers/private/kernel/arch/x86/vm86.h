/*
 * Copyright 2008 Jan Klötzke
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _VM86_H
#define _VM86_H


#ifndef __x86_64__

#include <OS.h>
#include <arch/x86/arch_cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VM86_MIN_RAM_SIZE 0x1000
#define RETURN_TO_32_INT  255

struct vm86_state {
	struct vm86_iframe regs;
	area_id bios_area;
	area_id ram_area;
	unsigned int if_flag : 1;
};

status_t x86_vm86_enter(struct vm86_iframe *frame);
void x86_vm86_return(struct vm86_iframe *frame, status_t retval);

status_t vm86_prepare(struct vm86_state *state, unsigned int ram_size);
void vm86_cleanup(struct vm86_state *state);
status_t vm86_do_int(struct vm86_state *state, uint8 vec);

#ifdef __cplusplus
}
#endif

#endif
#endif


