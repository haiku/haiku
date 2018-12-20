/*
 * Copyright 2003-2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Manuel J. Petit. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "runtime_loader_private.h"
#include "elf_tls.h"


// exported via the rld_export structure in user space program arguments


static image_id
export_load_add_on(char const *name, uint32 flags)
{
	void* handle;
	return load_library(name, flags, true, &handle);
}


static status_t
export_unload_add_on(image_id id)
{
	return unload_library(NULL, id, true);
}


static image_id
export_load_library(char const *name, uint32 flags, void **_handle)
{
	return load_library(name, flags, false, _handle);
}


static status_t
export_unload_library(void* handle)
{
	return unload_library(handle, -1, false);
}


status_t
reinit_after_fork()
{
	status_t returnstatus = B_OK;
	if (status_t status = elf_reinit_after_fork())
		returnstatus = status;
	if (status_t status = heap_reinit_after_fork())
		returnstatus = status;
	return returnstatus;
}


struct rld_export gRuntimeLoader = {
	// dynamic loading support API
	export_load_add_on,
	export_unload_add_on,
	export_load_library,
	export_unload_library,
	get_symbol,
	get_library_symbol,
	get_nth_symbol,
	get_nearest_symbol_at_address,
	test_executable,
	get_executable_architecture,
	get_next_image_dependency,
	get_tls_address,
	destroy_thread_tls,

	reinit_after_fork,
	NULL, // call_atexit_hooks_for_range
	terminate_program,

	// the following values will be set later
	NULL,	// program_args
	NULL,	// commpage_address
	0		// ABI version
};

rld_export* __gRuntimeLoader = &gRuntimeLoader;


void
rldexport_init(void)
{
	gRuntimeLoader.program_args = gProgramArgs;
	gRuntimeLoader.commpage_address = __gCommPageAddress;
}


/*!	Is called for all images, and sets the minimum ABI version found to the
	gRuntimeLoader.abi_version field.
*/
void
set_abi_version(int abi_version)
{
	if (gRuntimeLoader.abi_version == 0
		|| gRuntimeLoader.abi_version > abi_version) {
		gRuntimeLoader.abi_version = abi_version;
	}
}
