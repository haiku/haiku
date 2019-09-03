/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#include <thread.h>
#include <arch_thread.h>

#include <arch_cpu.h>
#include <arch/thread.h>
#include <boot/stage2.h>
#include <kernel.h>
#include <thread.h>
#include <tls.h>
#include <vm/vm_types.h>
#include <vm/VMAddressSpace.h>
#include <arch_vm.h>
#include <arch/vm_translation_map.h>

#include <string.h>

//#define TRACE_ARCH_THREAD
#ifdef TRACE_ARCH_THREAD
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


status_t
arch_thread_init(struct kernel_args *args)
{
	return B_OK;
}


status_t
arch_team_init_team_struct(Team *team, bool kernel)
{
	return B_OK;
}


status_t
arch_thread_init_thread_struct(Thread *thread)
{
	return B_OK;
}


void
arch_thread_init_kthread_stack(Thread* thread, void* _stack, void* _stackTop,
	void (*function)(void*), const void* data)
{
}


status_t
arch_thread_init_tls(Thread *thread)
{
	return 0;
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
arch_thread_enter_userspace(Thread *thread, addr_t entry,
	void *arg1, void *arg2)
{
	return B_ERROR;
}


bool
arch_on_signal_stack(Thread *thread)
{
	return false;
}


status_t
arch_setup_signal_frame(Thread *thread, struct sigaction *sa,
	struct signal_frame_data *signalFrameData)
{
	return B_ERROR;
}


int64
arch_restore_signal_frame(struct signal_frame_data* signalFrameData)
{
	return 0;
}


void
arch_check_syscall_restart(Thread *thread)
{
}


void
arch_store_fork_frame(struct arch_fork_arg *arg)
{
}


void
arch_restore_fork_frame(struct arch_fork_arg *arg)
{
}
