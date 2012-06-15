/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <arch/thread.h>

#include <team.h>
#include <thread.h>


status_t
arch_thread_init(struct kernel_args *args)
{
	return B_OK;
}


status_t
arch_team_init_team_struct(Team *p, bool kernel)
{
	return B_ERROR;
}


status_t
arch_thread_init_thread_struct(Thread *thread)
{
	return B_ERROR;
}


void
arch_thread_init_kthread_stack(Thread* thread, void* _stack, void* _stackTop,
	void (*function)(void*), const void* data)
{

}


status_t
arch_thread_init_tls(Thread *thread)
{
	return B_ERROR;
}


void
arch_thread_context_switch(Thread *from, Thread *to)
{

}


void
arch_thread_dump_info(void *info)
{

}


status_t
arch_thread_enter_userspace(Thread* thread, addr_t entry, void* args1,
	void* args2)
{
	return B_ERROR;
}


bool
arch_on_signal_stack(Thread *thread)
{
	return false;
}


status_t
arch_setup_signal_frame(Thread* thread, struct sigaction* action,
	struct signal_frame_data* signalFrameData)
{
	return B_ERROR;
}


int64
arch_restore_signal_frame(struct signal_frame_data* signalFrameData)
{
	return 0;
}


void
arch_store_fork_frame(struct arch_fork_arg *arg)
{

}


void
arch_restore_fork_frame(struct arch_fork_arg* arg)
{

}
