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

