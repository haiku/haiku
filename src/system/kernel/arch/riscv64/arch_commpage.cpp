/*
 * Copyright 2007, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <commpage.h>

#include <string.h>

#include <KernelExport.h>

#include <cpu.h>
#include <elf.h>
#include <smp.h>

#include "syscall_numbers.h"


extern "C" void arch_user_thread_exit();

typedef void (*SignalHandler)(int signal, siginfo_t* signalInfo,
	ucontext_t* ctx);


extern "C" void
arch_user_signal_handler(signal_frame_data* data)
{
	SignalHandler handler = (SignalHandler)data->handler;
	handler(data->info.si_signo, &data->info, &data->context);

	#define TO_STRING_LITERAL_HELPER(number)	#number
	#define TO_STRING_LITERAL(number)	TO_STRING_LITERAL_HELPER(number)

	// _kern_restore_signal_frame(data)
	asm volatile(
		"mv a0, %0;"
		"li t0, " TO_STRING_LITERAL(SYSCALL_RESTORE_SIGNAL_FRAME) ";"
		"ecall;"
		:: "r"(data)
	);

	#undef TO_STRING_LITERAL_HELPER
	#undef TO_STRING_LITERAL
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


status_t
arch_commpage_init(void)
{
	return B_OK;
}


status_t
arch_commpage_init_post_cpus(void)
{
	register_commpage_function("arch_user_signal_handler",
		COMMPAGE_ENTRY_RISCV64_SIGNAL_HANDLER, "commpage_signal_handler",
		(addr_t)&arch_user_signal_handler);

	register_commpage_function("arch_user_thread_exit",
		COMMPAGE_ENTRY_RISCV64_SIGNAL_THREAD_EXIT, "commpage_thread_exit",
		(addr_t)&arch_user_thread_exit);

	return B_OK;
}
