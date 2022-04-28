/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#include <commpage.h>

#include <string.h>

#include <KernelExport.h>

#include <cpu.h>
#include <elf.h>
#include <smp.h>


extern "C" void _thread_exit_syscall();


static void
register_commpage_function(const char* functionName, int32 commpageIndex,
	const char* commpageSymbolName, addr_t expectedAddress)
{
	// get address and size of function
	elf_symbol_info symbolInfo;
	if (elf_lookup_kernel_symbol(functionName, &symbolInfo) != B_OK) {
		panic("register_commpage_function(): Failed to find signal frame function \"%s\"!",
			functionName);
	}

	ASSERT(expectedAddress == symbolInfo.address);

	// fill in the commpage table entry
	addr_t position = fill_commpage_entry(commpageIndex, (void*)symbolInfo.address,
		symbolInfo.size);

	// add symbol to the commpage image
	image_id image = get_commpage_image();
	elf_add_memory_image_symbol(image, commpageSymbolName, position, symbolInfo.size,
		B_SYMBOL_TYPE_TEXT);
}


status_t
arch_commpage_init(void)
{
	return B_OK;
}


status_t
arch_commpage_init_post_cpus(void)
{
	register_commpage_function("_thread_exit_syscall", COMMPAGE_ENTRY_ARM64_THREAD_EXIT,
		"commpage_thread_exit", (addr_t)&_thread_exit_syscall);

	return B_OK;
}
