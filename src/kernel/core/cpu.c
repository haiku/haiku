/* This file contains the atomic_* and cpu functions (init, etc). */

/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel.h>
#include <cpu.h>
#include <vm.h>
#include <arch/cpu.h>
#include <stage2.h>
#include <atomic.h>

#include <string.h>

/* global per-cpu structure */
cpu_ent cpu[MAX_BOOT_CPUS];

int cpu_init(kernel_args *ka)
{
	int i;

	memset(cpu, 0, sizeof(cpu));
	for(i = 0; i < MAX_BOOT_CPUS; i++) {
		cpu[i].info.cpu_num = i;
	}

	return arch_cpu_init(ka);
}

int cpu_preboot_init(kernel_args *ka)
{
	return arch_cpu_preboot_init(ka);
}

int32 user_atomic_add(vint32 *uval, int32 incr)
{
	int32 val;
	int32 ret;

	if((addr)uval >= KERNEL_BASE && (addr)uval <= KERNEL_TOP)
		goto error;

	if(user_memcpy(&val, (int32 *)uval, sizeof(val)) < 0) 
		goto error;

	// XXX broken on non SH4-systems, or when interrupts are enabled
	// XXX x86 must use the assembly functions directly in userspace and not this ones
	ret = val;
	val += incr;

	if(user_memcpy((int32 *)uval, &val, sizeof(val)) < 0)
		goto error;

	return ret;

error:
	// XXX kill the app
	return -1;
}

int32 user_atomic_and(vint32 *uval, int32 incr)
{
	int val;
	int ret;

	if((addr)uval >= KERNEL_BASE && (addr)uval <= KERNEL_TOP)
		goto error;

	if(user_memcpy(&val, (int32 *)uval, sizeof(val)) < 0) 
		goto error;

	// XXX broken on non SH4-systems, or when interrupts are enabled
	// XXX x86 must use the assembly functions directly in userspace and not this ones
	ret = val;
	val &= incr;

	if(user_memcpy((int32 *)uval, &val, sizeof(val)) < 0)
		goto error;

	return ret;

error:
	// XXX kill the app
	return -1;
}

int32 user_atomic_or(vint32 *uval, int32 incr)
{
	int val;
	int ret;

	if((addr)uval >= KERNEL_BASE && (addr)uval <= KERNEL_TOP)
		goto error;

	if(user_memcpy(&val, (int32 *)uval, sizeof(val)) < 0) 
		goto error;

	// XXX broken on non SH4-systems, or when interrupts are enabled
	// XXX x86 must use the assembly functions directly in userspace and not this ones
	ret = val;
	val |= incr;

	if(user_memcpy((int32 *)uval, &val, sizeof(val)) < 0)
		goto error;

	return ret;

error:
	// XXX kill the app
	return -1;
}

int32 user_atomic_set(vint32 *uval, int32 set_to)
{
	int val;
	int ret;

	if((addr)uval >= KERNEL_BASE && (addr)uval <= KERNEL_TOP)
		goto error;

	if(user_memcpy(&val, (int32 *)uval, sizeof(val)) < 0) 
		goto error;

	// XXX broken on non SH4-systems, or when interrupts are enabled
	// XXX x86 must use the assembly functions directly in userspace and not this ones
	ret = val;
	val = set_to;

	if(user_memcpy((int32 *)uval, &val, sizeof(val)) < 0)
		goto error;

	return ret;

error:
	// XXX kill the app
	return -1;
}

int32 user_test_and_set(vint32 *uval, int32 set_to, int32 test_val)
{
	int val;
	int ret;

	if((addr)uval >= KERNEL_BASE && (addr)uval <= KERNEL_TOP)
		goto error;

	if(user_memcpy(&val, (int32 *)uval, sizeof(val)) < 0) 
		goto error;

	// XXX broken on non SH4-systems, or when interrupts are enabled
	// XXX x86 must use the assembly functions directly in userspace and not this ones
	ret = val;
	if(val == test_val) {
		val = set_to;
		if(user_memcpy((int32 *)uval, &val, sizeof(val)) < 0)
			goto error;
	}

	return ret;

error:
	// XXX kill the app
	return -1;
}

