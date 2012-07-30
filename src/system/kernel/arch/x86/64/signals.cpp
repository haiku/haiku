/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include "x86_signals.h"

#include <string.h>

#include <KernelExport.h>

#include <commpage.h>
#include <cpu.h>
#include <elf.h>
#include <smp.h>


extern "C" void _user_signal_handler(void);
extern int _user_signal_handler_end;


void
x86_initialize_commpage_signal_handler()
{
	void* handlerCode = (void*)&_user_signal_handler;
	void* handlerCodeEnd = &_user_signal_handler_end;

	// Copy the signal handler code to the commpage.
	size_t len = (size_t)((addr_t)handlerCodeEnd - (addr_t)handlerCode);
	fill_commpage_entry(COMMPAGE_ENTRY_X86_SIGNAL_HANDLER, handlerCode, len);

	// Add symbol to the commpage image.
	image_id image = get_commpage_image();
	elf_add_memory_image_symbol(image, "commpage_signal_handler",
		((addr_t*)USER_COMMPAGE_ADDR)[COMMPAGE_ENTRY_X86_SIGNAL_HANDLER],
		len, B_SYMBOL_TYPE_TEXT);
}

