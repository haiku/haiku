/*
** Copyright 2002-2004, The Haiku Team. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef KERNEL_ARCH_THREAD_H
#define KERNEL_ARCH_THREAD_H

#include <thread.h>

#ifdef __cplusplus
extern "C" {
#endif

int arch_team_init_team_struct(struct team *t, bool kernel);
int arch_thread_init_thread_struct(struct thread *t);
void arch_thread_init_tls(struct thread *thread);
void arch_thread_context_switch(struct thread *t_from, struct thread *t_to);
int arch_thread_init_kthread_stack(struct thread *t, int (*start_func)(void), void (*entry_func)(void), void (*exit_func)(void));
void arch_thread_dump_info(void *info);
void arch_thread_enter_uspace(struct thread *t, addr_t entry, void *args1, void *args2);
void arch_thread_switch_kstack_and_call(struct thread *t, addr_t new_kstack, void (*func)(void *), void *arg);

// ToDo: doing this this way is an ugly hack - please fix me!
//		(those functions are "static inline" for x86 - since
//		"extern inline" doesn't work for "gcc -g"...)
#ifndef ARCH_x86
static struct thread *arch_thread_get_current_thread(void);
static void arch_thread_set_current_thread(struct thread *t);
#endif

status_t arch_setup_signal_frame(struct thread *t, struct sigaction *sa, int sig, int sig_mask);
int64 arch_restore_signal_frame(void);
void arch_check_syscall_restart(struct thread *t);

void arch_store_fork_frame(struct arch_fork_arg *arg);
void arch_restore_fork_frame(struct arch_fork_arg *arg);

// for any inline overrides
#include <arch_thread.h>

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_ARCH_THREAD_H */
