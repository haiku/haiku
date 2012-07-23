/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
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


// implemented in assembly
extern "C" void x86_signal_frame_function_beos(signal_frame_data* frameData);


extern "C" void
x86_signal_frame_function(signal_frame_data* frameData)
{
	// Note: This function is copied to the commpage. Hence it needs to be
	// position independent. We don't build this source file with the respective
	// flags, but the code the compiler generates for this function is position
	// independent anyway. It simply doesn't contain constructs that could
	// result in position dependent code. The potentially problematic jumps
	// needed due to the "if" statement are all harmless relative jumps.

	if (frameData->siginfo_handler) {
		// SA_SIGINFO style handler function -- we additionally pass the user
		// data pointer
		void (*handler)(int, siginfo_t*, void*, void*)
			= (void (*)(int, siginfo_t*, void*, void*))frameData->handler;
		handler(frameData->info.si_signo, &frameData->info,
			&frameData->context, frameData->user_data);
	} else {
		// Simple handler function -- we call it with additional user data
		// pointer and vregs parameters. Note that unlike in BeOS the last
		// parameter is a pointer to a vregs structure, while in BeOS the
		// structure was passed be value. For setting up a BeOS binary
		// compatible signal handler call x86_signal_frame_function_beos() is
		// used instead.
		void (*handler)(int, void*, vregs*)
			= (void (*)(int, void*, vregs*))frameData->handler;
		handler(frameData->info.si_signo, frameData->user_data,
			&frameData->context.uc_mcontext);
	}

	#define TO_STRING_LITERAL_HELPER(number)	#number
	#define TO_STRING_LITERAL(number)	TO_STRING_LITERAL_HELPER(number)

	// call the restore_signal_frame() syscall -- does not return (here)
	asm volatile(
		// push frameData -- the parameter to restore_signal_frame()
		"pushl %0;"
		// push a dummy return value
		"pushl $0;"
		// syscall number to eax
		"movl $" TO_STRING_LITERAL(SYSCALL_RESTORE_SIGNAL_FRAME) ", %%eax;"
		// syscall
		"int $99;"
		:: "r"(frameData)
	);

	#undef TO_STRING_LITERAL_HELPER
	#undef TO_STRING_LITERAL
}


static void
register_signal_handler_function(const char* functionName, int32 commpageIndex,
	const char* commpageSymbolName, addr_t expectedAddress)
{
	// look up the x86_signal_frame_function() symbol -- we have its address,
	// but also need its size
	elf_symbol_info symbolInfo;
	if (elf_lookup_kernel_symbol(functionName, &symbolInfo)
			!= B_OK) {
		panic("x86_initialize_commpage_signal_handler(): Failed to find "
			"signal frame function \"%s\"!", functionName);
	}

	ASSERT(expectedAddress == symbolInfo.address);

	// fill in the commpage table entry
	fill_commpage_entry(commpageIndex, (void*)symbolInfo.address,
		symbolInfo.size);

	// add symbol to the commpage image
	image_id image = get_commpage_image();
	elf_add_memory_image_symbol(image, commpageSymbolName,
		((addr_t*)USER_COMMPAGE_ADDR)[commpageIndex], symbolInfo.size,
		B_SYMBOL_TYPE_TEXT);
}


void
x86_initialize_commpage_signal_handler()
{
	// standard handler
	register_signal_handler_function("x86_signal_frame_function",
		COMMPAGE_ENTRY_X86_SIGNAL_HANDLER, "commpage_signal_handler",
		(addr_t)&x86_signal_frame_function);

	// handler for BeOS backwards compatibility
	register_signal_handler_function("x86_signal_frame_function_beos",
		COMMPAGE_ENTRY_X86_SIGNAL_HANDLER_BEOS, "commpage_signal_handler_beos",
		(addr_t)&x86_signal_frame_function_beos);
}


addr_t
x86_get_user_signal_handler_wrapper(bool beosHandler)
{
	int32 index = beosHandler
		? COMMPAGE_ENTRY_X86_SIGNAL_HANDLER_BEOS
		: COMMPAGE_ENTRY_X86_SIGNAL_HANDLER;
	return ((addr_t*)USER_COMMPAGE_ADDR)[index];
}
