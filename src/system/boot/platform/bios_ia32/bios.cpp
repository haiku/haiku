/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2012-2017, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "bios.h"

#include <KernelExport.h>

#include "interrupts.h"


//#define TRACE_BIOS
#ifdef TRACE_BIOS
#   define TRACE(x) dprintf x
#else
#   define TRACE(x) ;
#endif


extern "C" void call_bios_internal(uint8 num, struct bios_regs* regs);


void
call_bios(uint8 num, struct bios_regs* regs)
{
	TRACE(("BIOS(%" B_PRIx8 "h): Restore BIOS IDT\n", num));
	restore_bios_idt();
	TRACE(("BIOS(%" B_PRIx8 "h): eax: 0x%" B_PRIx32 ", ebx: 0x%" B_PRIx32 ", "
		"ecx: 0x%" B_PRIx32 ", edx: 0x%" B_PRIx32 ", esi: 0x%" B_PRIx32 ", "
		"edi: 0x%" B_PRIx32 ", es: 0x%" B_PRIx16 ", flags: 0x%" B_PRIx8 "\n",
		num, regs->eax, regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi,
		regs->es, regs->flags));
	call_bios_internal(num, regs);
	TRACE(("BIOS(%" B_PRIx8 "h): Set debug BIOS IDT\n", num));
	set_debug_idt();
}
