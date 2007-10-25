/*
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <KernelExport.h>

#include <arch_platform.h>
#include <arch_thread.h>
#include <arch/cpu.h>
#include <boot/kernel_args.h>


status_t 
arch_cpu_preboot_init_percpu(kernel_args *args, int curr_cpu)
{
	// enable FPU
	//ppc:set_msr(get_msr() | MSR_FP_AVAILABLE);

	// The current thread must be NULL for all CPUs till we have threads.
	// Some boot code relies on this.
	arch_thread_set_current_thread(NULL);

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

#define CACHELINE 16

void 
arch_cpu_sync_icache(void *address, size_t len)
{
	int l, off;
	char *p;
	uint32 cacr;

	off = (unsigned int)address & (CACHELINE - 1);
	len += off;
	
	l = len;
	p = (char *)address - off;
	asm volatile ("movec %%cacr,%0" : "=r"(cacr):);
	cacr |= 0x00000004; /* ClearInstructionCacheEntry */
	do {
		/* the 030 invalidates only 1 long of the cache line */
		//XXX: what about 040 and 060 ?
		asm volatile ("movec %0,%%caar\n"		\
					  "movec %1,%%cacr\n"		\
					  "addq.l #4,%0\n"			\
					  "movec %0,%%caar\n"		\
					  "movec %1,%%cacr\n"		\
					  "addq.l #4,%0\n"			\
					  "movec %0,%%caar\n"		\
					  "movec %1,%%cacr\n"		\
					  "addq.l #4,%0\n"			\
					  "movec %0,%%caar\n"		\
					  "movec %1,%%cacr\n"		\
					  :: "r"(p), "r"(cacr));
		p += CACHELINE;
	} while ((l -= CACHELINE) > 0);
	m68k_nop();
}


void 
arch_cpu_invalidate_TLB_range(addr_t start, addr_t end)
{
	m68k_nop();
	while (start < end) {
		pflush(start);
		m68k_nop();
		start += B_PAGE_SIZE;
	}
	m68k_nop();
}


void 
arch_cpu_invalidate_TLB_list(addr_t pages[], int num_pages)
{
	int i;
	
	m68k_nop();
	for (i = 0; i < num_pages; i++) {
		pflush(pages[i]);
		m68k_nop();
	}
	m68k_nop();
}


void 
arch_cpu_global_TLB_invalidate(void)
{
	m68k_nop();
	pflusha();
	m68k_nop();
}


void 
arch_cpu_user_TLB_invalidate(void)
{
	// pflushfd ?
	arch_cpu_global_TLB_invalidate();
}


status_t
arch_cpu_user_memcpy(void *to, const void *from, size_t size,
	addr_t *faultHandler)
{
	char *tmp = (char *)to;
	char *s = (char *)from;
	addr_t oldFaultHandler = *faultHandler;

	if (m68k_set_fault_handler(faultHandler, (addr_t)&&error))
		goto error;

	while (size--)
		*tmp++ = *s++;

	*faultHandler = oldFaultHandler;
	return 0;

error:
	*faultHandler = oldFaultHandler;
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
	addr_t oldFaultHandler = *faultHandler;

	if (m68k_set_fault_handler(faultHandler, (addr_t)&&error))
		goto error;

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

	*faultHandler = oldFaultHandler;
	return from_length;

error:
	*faultHandler = oldFaultHandler;
	return B_BAD_ADDRESS;
}


status_t
arch_cpu_user_memset(void *s, char c, size_t count, addr_t *faultHandler)
{
	char *xs = (char *)s;
	addr_t oldFaultHandler = *faultHandler;

	if (m68k_set_fault_handler(faultHandler, (addr_t)&&error))
		goto error;

	while (count--)
		*xs++ = c;

	*faultHandler = oldFaultHandler;
	return 0;

error:
	*faultHandler = oldFaultHandler;
	return B_BAD_ADDRESS;
}


status_t
arch_cpu_shutdown(bool reboot)
{
	M68KPlatform::Default()->ShutDown(reboot);
	return B_ERROR;
}


void
arch_cpu_idle(void)
{
}


// The purpose of this function is to trick the compiler. When setting the
// page_handler to a label that is obviously (to the compiler) never used,
// it may reorganize the control flow, so that the labeled part is optimized
// away.
// By invoking the function like this
//
//	if (m68k_set_fault_handler(faultHandler, (addr_t)&&error))
//		goto error;
//
// the compiler has to keep the labeled code, since it can't guess the return
// value of this (non-inlinable) function. At least in my tests it worked that
// way, and I hope it will continue to work like this in the future.
//
bool
m68k_set_fault_handler(addr_t *handlerLocation, addr_t handler)
{
	*handlerLocation = handler;
	return false;
}
