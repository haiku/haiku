/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <kernel.h>
#include <thread.h>
#include <boot/stage2.h>


int
arch_team_init_team_struct(struct team *team, bool kernel)
{
	return 0;
}


int
arch_thread_init_thread_struct(struct thread *t)
{
	return 0;
}


int
arch_thread_init_kthread_stack(struct thread *t, int (*start_func)(void), void (*entry_func)(void), void (*exit_func)(void))
{
	return 0;
}


struct thread *
arch_thread_get_current_thread(void)
{
	return NULL;
}


void
arch_thread_set_current_thread(struct thread *thread)
{
}


void
arch_thread_init_tls(struct thread *thread)
{
}


void
arch_thread_switch_kstack_and_call(struct thread *t, addr new_kstack, void (*func)(void *), void *arg)
{
}


void
arch_thread_context_switch(struct thread *t_from, struct thread *t_to)
{
}


void
arch_thread_dump_info(void *info)
{
}


void
arch_thread_enter_uspace(struct thread *thread, addr entry, void *arg1, void *arg2)
{
}


void
arch_setup_signal_frame(struct thread *thread, struct sigaction *sa, int sig, int sigMask)
{
}


int64
arch_restore_signal_frame(void)
{
	return 0;
}


void
arch_check_syscall_restart(struct thread *thread)
{
}


