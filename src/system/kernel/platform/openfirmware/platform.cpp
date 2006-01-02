/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <platform.h>

#include <KernelExport.h>

#include <boot/kernel_args.h>
#include <platform/openfirmware/openfirmware.h>


static int
debug_command_of_exit(int argc, char **argv)
{
	of_exit();
	kprintf("of_exit() failed!\n");
	return 0;
}


static int
debug_command_of_enter(int argc, char **argv)
{
	of_call_client_function("enter", 0, 0);
	return 0;
}


status_t
platform_init(struct kernel_args *kernelArgs)
{
	return of_init(
		(int(*)(void*))kernelArgs->platform_args.openfirmware_entry);
}


status_t
platform_init_post_vm(struct kernel_args *kernelArgs)
{
	add_debugger_command("of_exit", &debug_command_of_exit,
		"Exit to the Open Firmware prompt. No way to get back into the OS!");
	add_debugger_command("of_enter", &debug_command_of_enter,
		"Enter a subordinate Open Firmware interpreter. Quitting it returns "
		"to KDL.");

	return B_OK;
}

