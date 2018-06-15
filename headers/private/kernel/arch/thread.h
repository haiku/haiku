/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef KERNEL_ARCH_THREAD_H
#define KERNEL_ARCH_THREAD_H


#include <thread_types.h>


#ifdef __cplusplus
extern "C" {
#endif

status_t arch_thread_init(struct kernel_args *args);
status_t arch_team_init_team_struct(Team *t, bool kernel);
status_t arch_thread_init_thread_struct(Thread *t);
status_t arch_thread_init_tls(Thread *thread);
void arch_thread_context_switch(Thread *t_from, Thread *t_to);
void arch_thread_init_kthread_stack(Thread *thread, void *stack,
	void *stackTop, void (*function)(void*), const void *data);
void arch_thread_dump_info(void *info);
status_t arch_thread_enter_userspace(Thread *t, addr_t entry,
	void *args1, void *args2);

bool arch_on_signal_stack(Thread *thread);
status_t arch_setup_signal_frame(Thread *thread, struct sigaction *action,
	struct signal_frame_data *signalFrameData);
int64 arch_restore_signal_frame(struct signal_frame_data* signalFrameData);

void arch_store_fork_frame(struct arch_fork_arg *arg);
void arch_restore_fork_frame(struct arch_fork_arg *arg);

#define arch_syscall_64_bit_return_value()
	// overridden by architectures that need special handling

#ifdef __cplusplus
}
#endif

// for any inline overrides
#include <arch_thread.h>

#endif	/* KERNEL_ARCH_THREAD_H */
