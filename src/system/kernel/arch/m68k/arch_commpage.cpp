/*
 * Copyright 2007, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <commpage.h>

#include <string.h>

#include <KernelExport.h>

#include <cpu.h>
#include <smp.h>


// user syscall assembly stubs
extern "C" void _user_syscall_int(void);
extern unsigned int _user_syscall_int_end;
extern "C" void _user_syscall_sysenter(void);
extern unsigned int _user_syscall_sysenter_end;


status_t
arch_commpage_init(void)
{
	panic("WRITEME");
	return B_NO_INIT;
	return B_OK;
}

