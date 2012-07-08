/*
 * Copyright 2007, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <KernelExport.h>

#include <arch/cpu.h>
#include <boot/kernel_args.h>
#include <commpage.h>
#include <elf.h>


int arch_cpu_type;
int arch_fpu_type;
int arch_mmu_type;
int arch_platform;

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
arch_cpu_init_percpu(kernel_args *args, int curr_cpu)
{
	if (curr_cpu != 0)
		panic("No SMP support on ARM yet!\n");

	return 0;
}


status_t
arch_cpu_init(kernel_args *args)
{
	arch_cpu_type = args->arch_args.cpu_type;
	arch_fpu_type = args->arch_args.fpu_type;
	arch_mmu_type = args->arch_args.mmu_type;
	arch_platform = args->arch_args.platform;
	arch_platform = args->arch_args.machine;

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
	// add the functions to the commpage image
	image_id image = get_commpage_image();

	return B_OK;
}


void
arch_cpu_idle(void)
{
	uint32 Rd = 0;
	asm volatile("mcr p15, 0, %[c7format], c7, c0, 4" : : [c7format] "r" (Rd) );
}


status_t
arch_cpu_shutdown(bool reboot)
{
	while(1)
		arch_cpu_idle();

	// never reached
	return B_ERROR;
}


void
arch_cpu_sync_icache(void *address, size_t len)
{
	uint32 Rd = 0;
	asm volatile ("mcr p15, 0, %[c7format], c7, c5, 0"
		: : [c7format] "r" (Rd) );
}


void
arch_cpu_memory_read_barrier(void)
{
	asm volatile ("" : : : "memory");
}


void
arch_cpu_memory_write_barrier(void)
{
	asm volatile ("" : : : "memory");
}


void
arch_cpu_invalidate_TLB_range(addr_t start, addr_t end)
{
	int32 num_pages = end / B_PAGE_SIZE - start / B_PAGE_SIZE;
	while (num_pages-- >= 0) {
		asm volatile ("mcr p15, 0, %[c8format], c8, c6, 1"
			: : [c8format] "r" (start) );
		start += B_PAGE_SIZE;
	}
}


void
arch_cpu_invalidate_TLB_list(addr_t pages[], int num_pages)
{
	for (int i = 0; i < num_pages; i++) {
		asm volatile ("mcr p15, 0, %[c8format], c8, c6, 1":
			: [c8format] "r" (pages[i]) );
	}
}


void
arch_cpu_global_TLB_invalidate(void)
{
	uint32 Rd = 0;
	asm volatile ("mcr p15, 0, %[c8format], c8, c7, 0"
		: : [c8format] "r" (Rd) );
}


void
arch_cpu_user_TLB_invalidate(void)
{/*
	cpu_ops.flush_insn_pipeline();
	cpu_ops.flush_atc_user();
	cpu_ops.flush_insn_pipeline();
*/
#warning WRITEME
}


// TODO: all functions that use fault handlers need to be implemented
// in assembly due to problems passing in label addresses in gcc4.
status_t
arch_cpu_user_memcpy(void *to, const void *from, size_t size,
	addr_t *faultHandler)
{
#warning WRITEME
/*
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
	*faultHandler = oldFaultHandler;*/
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
arch_cpu_user_strlcpy(char *to, const char *from,
	size_t size, addr_t *faultHandler)
{
#warning WRITEME
/*
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
	*faultHandler = oldFaultHandler;*/
	return B_BAD_ADDRESS;
}


status_t
arch_cpu_user_memset(void *s, char c, size_t count, addr_t *faultHandler)
{
#warning WRITEME

/*
	char *xs = (char *)s;
	addr_t oldFaultHandler = *faultHandler;

	if (m68k_set_fault_handler(faultHandler, (addr_t)&&error))
		goto error;

	while (count--)
		*xs++ = c;

	*faultHandler = oldFaultHandler;
	return 0;

error:
	*faultHandler = oldFaultHandler;*/

	return B_BAD_ADDRESS;
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
/*bool
m68k_set_fault_handler(addr_t *handlerLocation, addr_t handler)
{
	*handlerLocation = handler;
	return false;
}*/
