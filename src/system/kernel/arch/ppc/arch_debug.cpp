/* 
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <kernel.h>
#include <debug.h>
#include <arch/debug.h>


// ToDo: put stack trace and disassembly routines here


void
arch_debug_save_registers(int *regs)
{
}


void *
arch_debug_get_caller(void)
{
	// TODO: imeplement me
	return (void *)&arch_debug_get_caller;
}


status_t
arch_debug_init(kernel_args *args)
{
	return B_OK;
}


