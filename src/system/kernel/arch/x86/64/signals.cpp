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

#include "syscall_numbers.h"


extern "C" void __attribute__((noreturn))
x86_64_user_signal_handler(signal_frame_data* data)
{
	if (data->siginfo_handler) {
		auto handler = (void (*)(int, siginfo_t*, void*, void*))data->handler;
		handler(data->info.si_signo, &data->info, &data->context, data->user_data);
	} else {
		auto handler = (void (*)(int, void*, vregs*))data->handler;
		handler(data->info.si_signo, data->user_data, &data->context.uc_mcontext);
	}

	#define TO_STRING_LITERAL_HELPER(number)	#number
	#define TO_STRING_LITERAL(number)	TO_STRING_LITERAL_HELPER(number)

	// _kern_restore_signal_frame(data)
	asm volatile(
		"movq %0, %%rdi;"
		"movq $" TO_STRING_LITERAL(SYSCALL_RESTORE_SIGNAL_FRAME) ", %%rax;"
		"syscall;"
		:: "r"(data)
	);

	#undef TO_STRING_LITERAL_HELPER
	#undef TO_STRING_LITERAL

	__builtin_unreachable();
}


static void
register_commpage_function(const char* functionName, int32 commpageIndex,
	const char* commpageSymbolName, addr_t expectedAddress)
{
	// get address and size of function
	elf_symbol_info symbolInfo;
	if (elf_lookup_kernel_symbol(functionName, &symbolInfo)	!= B_OK) {
		panic("register_commpage_function(): Failed to find "
			"signal frame function \"%s\"!", functionName);
	}

	ASSERT(expectedAddress == symbolInfo.address);

	// fill in the commpage table entry
	addr_t position = fill_commpage_entry(commpageIndex,
		(void*)symbolInfo.address, symbolInfo.size);

	// add symbol to the commpage image
	image_id image = get_commpage_image();
	elf_add_memory_image_symbol(image, commpageSymbolName, position,
		symbolInfo.size, B_SYMBOL_TYPE_TEXT);
}


void
x86_initialize_commpage_signal_handler()
{
	register_commpage_function("x86_64_user_signal_handler",
		COMMPAGE_ENTRY_X86_SIGNAL_HANDLER, "commpage_signal_handler",
		(addr_t)&x86_64_user_signal_handler);
}
