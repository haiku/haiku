/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * Copyright 2007 François Revol, revol@free.fr
 * Copyright 2003-2005 Axel Dörfler, axeld@pinc-software.de
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <KernelExport.h>

//#include <arch_platform.h>
#include <arch_thread.h>
#include <arch/cpu.h>
#include <boot/kernel_args.h>
#include <commpage.h>
#include <elf.h>


int arch_cpu_type;
int arch_fpu_type;
int arch_mmu_type;
int arch_platform;

status_t 
arch_cpu_preboot_init_percpu(kernel_args* args, int curr_cpu)
{
#warning IMPLEMENT arch_cpu_preboot_init_percpu

	// The current thread must be NULL for all CPUs till we have threads.
	// Some boot code relies on this.
	arch_thread_set_current_thread(NULL);

	return B_ERROR;
}


status_t 
arch_cpu_init_percpu(kernel_args* args, int curr_cpu)
{
#warning IMPLEMENT arch_cpu_init_percpu
	//detect_cpu(curr_cpu);

	// we only support one anyway...
	return 0;
}


status_t
arch_cpu_init(kernel_args* args)
{
#warning IMPLEMENT arch_cpu_init
	arch_cpu_type = args->arch_args.cpu_type;
	arch_fpu_type = args->arch_args.fpu_type;
	arch_mmu_type = args->arch_args.mmu_type;
	arch_platform = args->arch_args.platform;
	arch_platform = args->arch_args.machine;

	return B_ERROR;
}


status_t
arch_cpu_init_post_vm(kernel_args* args)
{
#warning IMPLEMENT arch_cpu_init_post_vm
	return B_ERROR;
}


status_t
arch_cpu_init_post_modules(kernel_args* args)
{
#warning IMPLEMENT arch_cpu_init_post_modules
	return B_ERROR;
}


void 
arch_cpu_sync_icache(void* address, size_t len)
{
#warning IMPLEMENT arch_cpu_sync_icache
}


void
arch_cpu_memory_read_barrier(void)
{
#warning IMPLEMENT arch_cpu_memory_read_barrier
	asm volatile ("nop;" : : : "memory");
}


void
arch_cpu_memory_write_barrier(void)
{
#warning IMPLEMENT arch_cpu_memory_write_barrier
	asm volatile ("nop;" : : : "memory");
}


void 
arch_cpu_invalidate_TLB_range(addr_t start, addr_t end)
{
#warning IMPLEMENT arch_cpu_invalidate_TLB_range
}


void 
arch_cpu_invalidate_TLB_list(addr_t pages[], int num_pages)
{
#warning IMPLEMENT arch_cpu_invalidate_TLB_list
}


void 
arch_cpu_global_TLB_invalidate(void)
{
#warning IMPLEMENT arch_cpu_global_TLB_invalidate
}


void 
arch_cpu_user_TLB_invalidate(void)
{
#warning IMPLEMENT arch_cpu_user_TLB_invalidate
}


status_t
arch_cpu_user_memcpy(void* to, const void* from, size_t size,
	addr_t* faultHandler)
{
#warning IMPLEMENT arch_cpu_user_memcpy
	return B_BAD_ADDRESS;
}


ssize_t
arch_cpu_user_strlcpy(char* to, const char* from, size_t size,
	addr_t* faultHandler)
{
#warning IMPLEMENT arch_cpu_user_strlcpy
	return B_BAD_ADDRESS;
}


status_t
arch_cpu_user_memset(void* s, char c, size_t count, addr_t* faultHandler)
{
#warning IMPLEMENT arch_cpu_user_memset
	return B_BAD_ADDRESS;
}


status_t
arch_cpu_shutdown(bool reboot)
{
#warning IMPLEMENT arch_cpu_shutdown
	return B_ERROR;
}


void
arch_cpu_idle(void)
{
#warning IMPLEMENT arch_cpu_idle
}

