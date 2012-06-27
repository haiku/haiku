/*
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <cpu.h>

#include <boot/kernel_args.h>


status_t
arch_cpu_preboot_init_percpu(kernel_args* args, int cpu)
{
	return B_OK;
}


status_t
arch_cpu_init_percpu(kernel_args* args, int cpu)
{
	return B_OK;
}


status_t
arch_cpu_init(kernel_args* args)
{
	return B_OK;
}


status_t
arch_cpu_init_post_vm(kernel_args* args)
{
	return B_OK;
}


status_t
arch_cpu_init_post_modules(kernel_args* args)
{
	return B_OK;
}


void
arch_cpu_user_TLB_invalidate(void)
{

}


void
arch_cpu_global_TLB_invalidate(void)
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


ssize_t
arch_cpu_user_strlcpy(char* to, const char* from, size_t size,
	addr_t* faultHandler)
{
	int fromLength = 0;
	addr_t oldFaultHandler = *faultHandler;

	// this check is to trick the gcc4 compiler and have it keep the error label
	if (to == NULL && size > 0)
		goto error;

	*faultHandler = (addr_t)&&error;

	if (size > 0) {
		to[--size] = '\0';
		// copy
		for ( ; size; size--, fromLength++, to++, from++) {
			if ((*to = *from) == '\0')
				break;
		}
	}

	// count any leftover from chars
	while (*from++ != '\0') {
		fromLength++;
	}

	*faultHandler = oldFaultHandler;
	return fromLength;

error:
	*faultHandler = oldFaultHandler;
	return B_BAD_ADDRESS;
}


status_t
arch_cpu_user_memcpy(void* to, const void* from, size_t size,
	addr_t* faultHandler)
{
	char* d = (char*)to;
	const char* s = (const char*)from;
	addr_t oldFaultHandler = *faultHandler;

	// this check is to trick the gcc4 compiler and have it keep the error label
	if (s == NULL)
		goto error;

	*faultHandler = (addr_t)&&error;

	for (; size != 0; size--) {
		*d++ = *s++;
	}

	*faultHandler = oldFaultHandler;
	return 0;

error:
	*faultHandler = oldFaultHandler;
	return B_BAD_ADDRESS;
}


status_t
arch_cpu_user_memset(void* s, char c, size_t count, addr_t* faultHandler)
{
	char* xs = (char*)s;
	addr_t oldFaultHandler = *faultHandler;

	// this check is to trick the gcc4 compiler and have it keep the error label
	if (s == NULL)
		goto error;

	*faultHandler = (addr_t)&&error;

	while (count--)
		*xs++ = c;

	*faultHandler = oldFaultHandler;
	return 0;

error:
	*faultHandler = oldFaultHandler;
	return B_BAD_ADDRESS;
}


status_t
arch_cpu_shutdown(bool rebootSystem)
{
	return B_ERROR;
}


void
arch_cpu_idle(void)
{
	asm("hlt");
}


void
arch_cpu_sync_icache(void* address, size_t length)
{
	// Instruction cache is always consistent on x86.
}


void
arch_cpu_memory_read_barrier(void)
{
	asm volatile("lfence" : : : "memory");
}


void
arch_cpu_memory_write_barrier(void)
{
	asm volatile("sfence" : : : "memory");
}

