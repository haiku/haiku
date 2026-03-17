/*
 * Copyright 2019-2022 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#include <thread.h>
#include <arch_thread.h>

#include <arch_cpu.h>
#include <arch/thread.h>
#include <boot/stage2.h>
#include <commpage_defs.h>
#include <kernel.h>
#include <thread.h>
#include <tls.h>
#include <vm/vm_types.h>
#include <vm/VMAddressSpace.h>
#include <arch_vm.h>
#include <arch/vm.h>
#include <arch/vm_translation_map.h>

#include <string.h>

//#define TRACE_ARCH_THREAD
#ifdef TRACE_ARCH_THREAD
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


void
arm64_push_iframe(struct iframe_stack *stack, struct iframe *frame)
{
	ASSERT(stack->index < IFRAME_TRACE_DEPTH);
	stack->frames[stack->index++] = frame;
}


void
arm64_pop_iframe(struct iframe_stack *stack)
{
	ASSERT(stack->index > 0);
	stack->index--;
}


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
	memset(&thread->arch_info, 0, sizeof(arch_thread));
	thread->arch_info.regs[10] = (uint64_t)data;
	thread->arch_info.regs[11] = (uint64_t)function;
	thread->arch_info.regs[12] = (uint64_t)_stackTop;
}


status_t
arch_thread_init_tls(Thread *thread)
{
	thread->user_local_storage =
		thread->user_stack_base + thread->user_stack_size;
	return B_OK;
}

extern "C" void _arch_context_swap(arch_thread *from, arch_thread *to);


void
arch_thread_context_switch(Thread *from, Thread *to)
{
	arch_vm_aspace_swap(from->team->address_space, to->team->address_space);
	_arch_context_swap(&from->arch_info, &to->arch_info);
}


void
arch_thread_dump_info(void *info)
{
}


extern "C" void _eret_with_iframe(iframe *frame);


status_t
arch_thread_enter_userspace(Thread *thread, addr_t entry,
	void *arg1, void *arg2)
{
	addr_t threadExitAddr;
	{
		addr_t commpageAdr = (addr_t)thread->team->commpage_address;
		status_t ret = user_memcpy(&threadExitAddr,
			&((addr_t*)commpageAdr)[COMMPAGE_ENTRY_ARM64_THREAD_EXIT],
			sizeof(threadExitAddr));
		ASSERT(ret == B_OK);
		threadExitAddr += commpageAdr;
	}

	iframe frame;
	memset(&frame, 0, sizeof(frame));

	frame.spsr = 0;
	frame.elr = entry;
	frame.x[0] = (uint64_t)arg1;
	frame.x[1] = (uint64_t)arg2;
	frame.lr = threadExitAddr;
	frame.sp = thread->user_stack_base + thread->user_stack_size;
	frame.tpidr = thread->user_local_storage;

	thread->arch_info.iframes.index = 0;

	_eret_with_iframe(&frame);
	return B_ERROR;
}


bool
arch_on_signal_stack(Thread *thread)
{
	iframe_stack* iframes = &thread->arch_info.iframes;
	iframe* frame = iframes->frames[iframes->index - 1];

	return frame->sp >= thread->signal_stack_base
		&& frame->sp < thread->signal_stack_base
			+ thread->signal_stack_size;
}


static uint8*
get_signal_stack(Thread* thread, struct iframe* frame,
	struct sigaction* action, size_t spaceNeeded)
{
	// use the alternate signal stack if we should and can
	if (
		thread->signal_stack_enabled &&
		(action->sa_flags & SA_ONSTACK) != 0 && (
			frame->sp < thread->signal_stack_base ||
			frame->sp >= thread->signal_stack_base + thread->signal_stack_size
		)
	) {
		addr_t stackTop = thread->signal_stack_base
			+ thread->signal_stack_size;
		return (uint8*)ROUNDDOWN(stackTop - spaceNeeded, 16);
	}
	return (uint8*)ROUNDDOWN(frame->sp - spaceNeeded, 16);
}


status_t
arch_setup_signal_frame(Thread *thread, struct sigaction *sa,
	struct signal_frame_data *signalFrameData)
{
	iframe_stack* iframes = &thread->arch_info.iframes;
	iframe *frame = iframes->frames[iframes->index - 1];

	memcpy(signalFrameData->context.uc_mcontext.x, frame->x, sizeof(frame->x));
	signalFrameData->context.uc_mcontext.x[29] = frame->fp;
	signalFrameData->context.uc_mcontext.lr = frame->lr;
	signalFrameData->context.uc_mcontext.elr = frame->elr;
	signalFrameData->context.uc_mcontext.spsr = frame->spsr;
	signalFrameData->context.uc_mcontext.sp = frame->sp;
	memcpy(signalFrameData->context.uc_mcontext.fp_q, frame->fpu.regs,
		sizeof(signalFrameData->context.uc_mcontext.fp_q));
	signalFrameData->context.uc_mcontext.fpsr = frame->fpu.fpsr;
	signalFrameData->context.uc_mcontext.fpcr = frame->fpu.fpcr;
	signalFrameData->syscall_restart_return_value = thread->arch_info.old_x0;

	signal_get_user_stack(frame->sp, &signalFrameData->context.uc_stack);

	uint8* userStack = get_signal_stack(thread, frame, sa,
		sizeof(*signalFrameData));
	status_t res = user_memcpy(userStack, signalFrameData,
		sizeof(*signalFrameData));
	if (res < B_OK)
		return res;

	addr_t commpageAddr = (addr_t)thread->team->commpage_address;
	addr_t signalHandlerAddr;
	ASSERT(user_memcpy(&signalHandlerAddr,
		&((addr_t*)commpageAddr)[COMMPAGE_ENTRY_ARM64_SIGNAL_HANDLER],
		sizeof(signalHandlerAddr)) >= B_OK);
	signalHandlerAddr += commpageAddr;

	frame->lr = frame->elr;
	frame->sp = (addr_t)userStack;
	frame->elr = signalHandlerAddr;
	frame->x[0] = frame->sp;

	return B_OK;
}


int64
arch_restore_signal_frame(struct signal_frame_data* signalFrameData)
{
	iframe_stack* iframes = &thread_get_current_thread()->arch_info.iframes;
	iframe *frame = iframes->frames[iframes->index - 1];

	thread_get_current_thread()->arch_info.old_x0
		= signalFrameData->syscall_restart_return_value;

	memcpy(frame->x, signalFrameData->context.uc_mcontext.x, sizeof(frame->x));
	frame->fp = signalFrameData->context.uc_mcontext.x[29];
	frame->lr = signalFrameData->context.uc_mcontext.lr;
	frame->elr = signalFrameData->context.uc_mcontext.elr;
	frame->spsr = signalFrameData->context.uc_mcontext.spsr;
	frame->sp = signalFrameData->context.uc_mcontext.sp;
	memcpy(frame->fpu.regs, signalFrameData->context.uc_mcontext.fp_q,
		sizeof(signalFrameData->context.uc_mcontext.fp_q));
	frame->fpu.fpsr = signalFrameData->context.uc_mcontext.fpsr;
	frame->fpu.fpcr = signalFrameData->context.uc_mcontext.fpcr;

	return frame->x[0];
}


void
arch_store_fork_frame(struct arch_fork_arg *arg)
{
	iframe_stack* iframes = &thread_get_current_thread()->arch_info.iframes;
	memcpy(&arg->frame, iframes->frames[iframes->index - 1], sizeof(iframe));
	arg->frame.x[0] = 0;
}


void
arch_restore_fork_frame(struct arch_fork_arg *arg)
{
	_eret_with_iframe(&arg->frame);
}
