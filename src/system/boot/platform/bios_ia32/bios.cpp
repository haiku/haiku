/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "bios.h"

#include "interrupts.h"


extern "C" void call_bios_internal(uint8 num, struct bios_regs* regs);


void
call_bios(uint8 num, struct bios_regs* regs)
{
	restore_bios_idt();
	call_bios_internal(num, regs);
	set_debug_idt();

}
