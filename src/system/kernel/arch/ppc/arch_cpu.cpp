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
#include <arch/thread.h>
#include <boot/kernel_args.h>

static bool sHasTlbia;

status_t
arch_cpu_preboot_init_percpu(kernel_args *args, int curr_cpu)
{
	// enable FPU
	set_msr(get_msr() | MSR_FP_AVAILABLE);

	// The current thread must be NULL for all CPUs till we have threads.
	// Some boot code relies on this.
	arch_thread_set_current_thread(NULL);

	return B_OK;
}


status_t
arch_cpu_init(kernel_args *args)
{
	// TODO: Let the boot loader put that info into the kernel args
	// (property "tlbia" in the CPU node).
	sHasTlbia = false;

	return B_OK;
}


status_t
arch_cpu_init_post_vm(kernel_args *args)
{
	return B_OK;
}

status_t
arch_cpu_init_percpu(kernel_args *args, int curr_cpu)
{
        //detect_cpu(curr_cpu);

        // we only support one on ppc anyway at the moment...
	//XXX: WRITEME
        return 0;
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
arch_cpu_memory_read_barrier(void)
{
// WARNING PPC: is it model-dependant ?
	asm volatile ("lwsync");
}


void
arch_cpu_memory_write_barrier(void)
{
// WARNING PPC: is it model-dependant ?
	asm volatile ("isync");
	asm volatile ("eieio");
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
	if (sHasTlbia) {
		ppc_sync();
		tlbia();
		ppc_sync();
	} else {
		addr_t address = 0;
		unsigned long i;

		ppc_sync();
		for (i = 0; i < 0x100000; i++) {
			tlbie(address);
			eieio();
			ppc_sync();

			address += B_PAGE_SIZE;
		}
		tlbsync();
		ppc_sync();
	}
}


void
arch_cpu_user_TLB_invalidate(void)
{
	arch_cpu_global_TLB_invalidate();
}


status_t
arch_cpu_user_memcpy(void *to, const void *from, size_t size,
	addr_t *faultHandler)
{
	char *tmp = (char *)to;
	char *s = (char *)from;
	addr_t oldFaultHandler = *faultHandler;

// TODO: This doesn't work correctly with gcc 4 anymore!
	if (ppc_set_fault_handler(faultHandler, (addr_t)&&error))
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

// TODO: This doesn't work correctly with gcc 4 anymore!
	if (ppc_set_fault_handler(faultHandler, (addr_t)&&error))
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

// TODO: This doesn't work correctly with gcc 4 anymore!
	if (ppc_set_fault_handler(faultHandler, (addr_t)&&error))
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
	PPCPlatform::Default()->ShutDown(reboot);
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
//	if (ppc_set_fault_handler(faultHandler, (addr_t)&&error))
//		goto error;
//
// the compiler has to keep the labeled code, since it can't guess the return
// value of this (non-inlinable) function. At least in my tests it worked that
// way, and I hope it will continue to work like this in the future.
//
bool
ppc_set_fault_handler(addr_t *handlerLocation, addr_t handler)
{
// TODO: This doesn't work correctly with gcc 4 anymore!
	*handlerLocation = handler;
	return false;
}
