/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>

#include <arch/cpu.h>
#include <boot/kernel_args.h>
#include <commpage.h>
#include <elf.h>


status_t
arch_cpu_preboot_init_percpu(kernel_args *args, int curr_cpu)
{
	return B_OK;
}


status_t
arch_cpu_init_percpu(kernel_args *args, int curr_cpu)
{
	return 0;
}


status_t
arch_cpu_init(kernel_args *args)
{
	return B_OK;
}


status_t
arch_cpu_init_post_vm(kernel_args *args)
{
	return B_OK;
}


status_t
arch_cpu_init_post_modules(kernel_args *args)
{
	return B_OK;
}


status_t
arch_cpu_shutdown(bool reboot)
{
	// never reached
	return B_ERROR;
}


void
arch_cpu_sync_icache(void *address, size_t len)
{
}


void
arch_cpu_memory_read_barrier(void)
{
}


void
arch_cpu_memory_write_barrier(void)
{
}


void
arch_cpu_invalidate_TLB_range(addr_t start, addr_t end)
{
}


void
arch_cpu_invalidate_TLB_list(addr_t pages[], int num_pages)
{
}


void
arch_cpu_global_TLB_invalidate(void)
{
}


void
arch_cpu_user_TLB_invalidate(void)
{
}
