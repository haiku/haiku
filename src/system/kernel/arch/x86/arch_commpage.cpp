/*
 * Copyright 2008-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2007, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <commpage.h>

#include "x86_syscalls.h"


status_t
arch_commpage_init(void)
{
	return B_OK;
}


status_t
arch_commpage_init_post_cpus(void)
{
	// select the optimum syscall mechanism and patch the commpage
	x86_initialize_commpage_syscall();

	return B_OK;
}
