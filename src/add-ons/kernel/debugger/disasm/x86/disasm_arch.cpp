/*
 * Copyright 2008, François Revol, revol@free.fr
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>
#include <KernelExport.h>

#include "disasm_arch.h"
#include "udis86.h"

static ud_t sUDState;
static void (*sSyntax)(ud_t *) = UD_SYN_ATT;
static unsigned int sVendor = UD_VENDOR_INTEL;

extern "C" void
disasm_arch_assert(const char *condition)
{
	kprintf("assert: %s\n", condition);
}


status_t
disasm_arch_dump_insns(addr_t where, size_t length, int count)
{
	//status_t err;
	int i;
	
	ud_set_input_buffer(&sUDState, (unsigned char *)where, length);
	ud_set_mode(&sUDState, 32);
	ud_set_pc(&sUDState, (uint64_t)where);
	ud_set_syntax(&sUDState, sSyntax);
	ud_set_vendor(&sUDState, sVendor);
	for (i = 0; i < count; i++) {
		int ret;
		ret = ud_disassemble(&sUDState);
		if (ret < 1)
			break;
		// TODO: dig operands and lookup symbols
		kprintf("0x%08lx: %16.16s\t%s\n", 
			(uint32)(/*where +*/ ud_insn_off(&sUDState)),
			ud_insn_hex(&sUDState),
			ud_insn_asm(&sUDState));
	}
	return B_OK;
}


status_t
disasm_arch_init()
{
	ud_init(&sUDState);
	// XXX: check for AMD and set sVendor;
	return B_OK;
}

status_t
disasm_arch_fini()
{
	return B_OK;
}


