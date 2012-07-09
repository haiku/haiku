/*
 * Copyright 2002-2011, The Haiku Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_x86_THREAD_H
#define _KERNEL_ARCH_x86_THREAD_H


#include <arch/cpu.h>


#ifdef __cplusplus
extern "C" {
#endif

struct sigaction;


struct iframe* x86_get_user_iframe(void);
struct iframe* x86_get_current_iframe(void);
struct iframe* x86_get_thread_user_iframe(Thread* thread);

phys_addr_t x86_next_page_directory(Thread* from, Thread* to);
void x86_initial_return_to_userland(Thread* thread, struct iframe* iframe);
uint8* x86_get_signal_stack(Thread* thread, struct iframe* frame,
	struct sigaction* action);

void x86_restart_syscall(struct iframe* frame);
void x86_set_tls_context(Thread* thread);


#ifdef __x86_64__


static inline Thread*
arch_thread_get_current_thread(void)
{
	addr_t addr;
	__asm__("mov %%gs:0, %0" : "=r"(addr));
	return (Thread*)addr;
}


static inline void
arch_thread_set_current_thread(Thread* t)
{
	// Point GS segment base at thread architecture data.
	t->arch_info.thread = t;
	x86_write_msr(IA32_MSR_GS_BASE, (addr_t)&t->arch_info);
}


#else	// __x86_64__


// override empty macro
#undef arch_syscall_64_bit_return_value
void arch_syscall_64_bit_return_value(void);


static inline Thread*
arch_thread_get_current_thread(void)
{
	Thread* t = (Thread*)x86_read_dr3();
	return t;
}


static inline void
arch_thread_set_current_thread(Thread* t)
{
	x86_write_dr3(t);
}


#endif	// __x86_64__


#ifdef __cplusplus
}
#endif

#endif /* _KERNEL_ARCH_x86_THREAD_H */
