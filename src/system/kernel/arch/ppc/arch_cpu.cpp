/*
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <KernelExport.h>

#include <arch_platform.h>
#include <arch/cpu.h>
#include <boot/kernel_args.h>


status_t 
arch_cpu_preboot_init(kernel_args *args)
{
	return B_OK;
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

#define CACHELINE 32

void 
arch_cpu_sync_icache(void *address, size_t len)
{
	int l, off;
	char *p;

	off = (unsigned int)address & (CACHELINE - 1);
	len += off;

	l = len;
	p = (char *)address - off;
	do {
		asm volatile ("dcbst 0,%0" :: "r"(p));
		p += CACHELINE;
	} while ((l -= CACHELINE) > 0);
	asm volatile ("sync");

	p = (char *)address - off;
	do {
		asm volatile ("icbi 0,%0" :: "r"(p));
		p += CACHELINE;
	} while ((len -= CACHELINE) > 0);
	asm volatile ("sync");
	isync();
}


void 
arch_cpu_invalidate_TLB_range(addr_t start, addr_t end)
{
	asm volatile("sync");
	while (start < end) {
		asm volatile("tlbie %0" :: "r" (start));
		asm volatile("eieio");
		asm volatile("sync");
		start += B_PAGE_SIZE;
	}
	asm volatile("tlbsync");
	asm volatile("sync");
}


void 
arch_cpu_invalidate_TLB_list(addr_t pages[], int num_pages)
{
	int i;

	asm volatile("sync");
	for (i = 0; i < num_pages; i++) {
		asm volatile("tlbie %0" :: "r" (pages[i]));
		asm volatile("eieio");
		asm volatile("sync");
	}
	asm volatile("tlbsync");
	asm volatile("sync");
}


void 
arch_cpu_global_TLB_invalidate(void)
{
	ppc_sync();
	tlbia();
	ppc_sync();
}


void 
arch_cpu_user_TLB_invalidate(void)
{
	ppc_sync();
	tlbia();
	ppc_sync();
}


status_t
arch_cpu_user_memcpy(void *to, const void *from, size_t size, addr_t *fault_handler)
{
	char *tmp = (char *)to;
	char *s = (char *)from;

	*fault_handler = (addr_t)&&error;

	while (size--)
		*tmp++ = *s++;

	*fault_handler = 0;
	return 0;

error:
	*fault_handler = 0;
	return B_BAD_ADDRESS;
}


/**	\brief Copies at most (\a size - 1) characters from the string in \a from to
 *	the string in \a to, NULL-terminating the result.
 *
 *	\param to Pointer to the destination C-string.
 *	\param from Pointer to the source C-string.
 *	\param size Size in bytes of the string buffer pointed to by \a to.
 *	
 *	\return strlen(\a from).
 */

ssize_t
arch_cpu_user_strlcpy(char *to, const char *from, size_t size, addr_t *faultHandler)
{
	int from_length = 0;

	*faultHandler = (addr_t)&&error;

	if (size > 0) {
		to[--size] = '\0';
		// copy 
		for ( ; size; size--, from_length++, to++, from++) {
			if ((*to = *from) == '\0')
				break;
		}
	}
	// count any leftover from chars
	while (*from++ != '\0')
		from_length++;

	*faultHandler = 0;
	return from_length;

error:
	*faultHandler = 0;
	return B_BAD_ADDRESS;
}


status_t
arch_cpu_user_memset(void *s, char c, size_t count, addr_t *fault_handler)
{
	char *xs = (char *)s;

	*fault_handler = (addr_t)&&error;

	while (count--)
		*xs++ = c;

	*fault_handler = 0;
	return 0;

error:
	*fault_handler = 0;
	return B_BAD_ADDRESS;
}


status_t
arch_cpu_shutdown(bool reboot)
{
	PPCPlatform::Default()->ShutDown(reboot);
	return B_ERROR;
}


void
arch_cpu_idle(void)
{
}

