/*
 * Copyright 2005-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Axel Dörfler <axeld@pinc-software.de>
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */
#ifndef _KERNEL_ARCH_M68K_INT_H
#define _KERNEL_ARCH_M68K_INT_H

#include <SupportDefs.h>

#define NUM_IO_VECTORS	256

/* The sprg0 register of each CPU points to the physical address of such
   a structure. So it is at hand in the early exception handling code.
*/
struct m68k_cpu_exception_context {
	void	*kernel_handle_exception;	// exception handler routine in the
										// kernel
	void	*exception_context;			// the virtual address of this
										// structure
	void	*kernel_stack;				// kernel stack for the current thread

	uint32	scratch[8];					// scratch memory for free use in the
										// early exception handling code
};

#ifdef __cplusplus
extern "C" {
#endif


struct m68k_cpu_exception_context *m68k_get_cpu_exception_context(int cpu);

void m68k_set_current_cpu_exception_context(
	struct m68k_cpu_exception_context *context);
		// only called once per CPU


#ifdef __cplusplus
}
#endif

#endif /* _KERNEL_ARCH_M68K_INT_H */
