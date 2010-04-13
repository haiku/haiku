/*
 * Copyright 2008, François Revol, revol@free.fr
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>
#include <KernelExport.h>

#include <debug.h>

#include "disasm_arch.h"
#include "udis86.h"


static ud_t sUDState;
static addr_t sCurrentReadAddress;
static void (*sSyntax)(ud_t *) = UD_SYN_ATT;
static unsigned int sVendor = UD_VENDOR_INTEL;


static int
read_next_byte(struct ud*)
{
	uint8_t buffer;
	if (debug_memcpy(B_CURRENT_TEAM, &buffer, (void*)sCurrentReadAddress, 1)
			!= B_OK) {
		kprintf("<read fault>\n");
		return UD_EOI;
	}

	sCurrentReadAddress++;
	return buffer;
}


static void
setup_disassembler(addr_t where)
{
	ud_set_input_hook(&sUDState, &read_next_byte);
	sCurrentReadAddress	= where;
	ud_set_mode(&sUDState, 32);
	ud_set_pc(&sUDState, (uint64_t)where);
	ud_set_syntax(&sUDState, sSyntax);
	ud_set_vendor(&sUDState, sVendor);
}


extern "C" void
disasm_arch_assert(const char *condition)
{
	kprintf("assert: %s\n", condition);
}


status_t
disasm_arch_dump_insns(addr_t where, int count, addr_t baseAddress,
	int backCount)
{
	int skipCount = 0;

	if (backCount > 0) {
		// count the instructions from base address to start address
		setup_disassembler(baseAddress);
		addr_t address = baseAddress;
		int baseCount = 0;
		int len;
		while (address < where && (len = ud_disassemble(&sUDState)) >= 1) {
			address += len;
			baseCount++;
		}

		if (address == where) {
			if (baseCount > backCount)
				skipCount = baseCount - backCount;
			count += baseCount;
		} else
			baseAddress = where;
	} else
		baseAddress = where;

	setup_disassembler(baseAddress);

	for (int i = 0; i < count; i++) {
		int ret;
		ret = ud_disassemble(&sUDState);
		if (ret < 1)
			break;

		if (skipCount > 0) {
			skipCount--;
			continue;
		}

		addr_t address = (addr_t)ud_insn_off(&sUDState);
		if (address == where)
			kprintf("\x1b[34m");

		// TODO: dig operands and lookup symbols
		kprintf("0x%08lx: %16.16s\t%s\n", address, ud_insn_hex(&sUDState),
			ud_insn_asm(&sUDState));

		if (address == where)
			kprintf("\x1b[m");
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


