/* Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under the terms of the MIT License.
 */


#include <string.h>

#include <arch_cpu.h>
#include <arch_debug.h>
#include <arch/thread.h>
#include <boot/stage2.h>
#include <commpage.h>
#include <kernel.h>
#include <thread.h>
#include <team.h>
#include <vm/vm_types.h>
#include <vm/VMAddressSpace.h>

#include "RISCV64VMTranslationMap.h"


extern "C" void SVecU();


status_t
arch_thread_init(struct kernel_args *args)
{
	// Initialize the static initial arch_thread state (sInitialState).
	// Currently nothing to do, i.e. zero initialized is just fine.

	return B_OK;
}


status_t
arch_team_init_team_struct(Team *team, bool kernel)
{
	// Nothing to do. The structure is empty.
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
	memset(&thread->arch_info.context, 0, sizeof(arch_context));
	thread->arch_info.context.sp = (addr_t)_stackTop;
	thread->arch_info.context.s[0] = 0; // fp
	thread->arch_info.context.s[1] = (addr_t)function;
	thread->arch_info.context.s[2] = (addr_t)data;
	thread->arch_info.context.ra = (addr_t)arch_thread_entry;

	memset(&thread->arch_info.fpuContext, 0, sizeof(fpu_context));
}


status_t
arch_thread_init_tls(Thread *thread)
{
	thread->user_local_storage =
		thread->user_stack_base + thread->user_stack_size;
	return B_OK;
}


void
arch_thread_context_switch(Thread *from, Thread *to)
{
	/*
	dprintf("arch_thread_context_switch(%p(%s), %p(%s))\n", from, from->name,
		to, to->name);
	*/

	auto fromMap = (RISCV64VMTranslationMap*)from->team->address_space->TranslationMap();
	auto toMap = (RISCV64VMTranslationMap*)to->team->address_space->TranslationMap();

	int cpu = to->cpu->cpu_num;
	toMap->ActiveOnCpus().SetBitAtomic(cpu);
	fromMap->ActiveOnCpus().ClearBitAtomic(cpu);

	// TODO: save/restore FPU only if needed
	save_fpu(&from->arch_info.fpuContext);
	restore_fpu(&to->arch_info.fpuContext);

	SetSatp(toMap->Satp());
	FlushTlbAllAsid(0);

	arch_context_switch(&from->arch_info.context, &to->arch_info.context);
}


void
arch_thread_dump_info(void *info)
{
}


status_t
arch_thread_enter_userspace(Thread *thread, addr_t entry, void *arg1,
	void *arg2)
{
	//dprintf("arch_thread_enter_uspace(%" B_PRId32 "(%s))\n", thread->id, thread->name);

	addr_t commpageAdr = (addr_t)thread->team->commpage_address;
	addr_t threadExitAddr;
	ASSERT(user_memcpy(&threadExitAddr,
		&((addr_t*)commpageAdr)[COMMPAGE_ENTRY_RISCV64_THREAD_EXIT],
		sizeof(threadExitAddr)) >= B_OK);
	threadExitAddr += commpageAdr;

	disable_interrupts();

	arch_stack* stackHeader = (arch_stack*)thread->kernel_stack_top - 1;
	stackHeader->thread = thread;

	iframe frame;
	memset(&frame, 0, sizeof(frame));

	SstatusReg status{.val = Sstatus()};
	status.pie = (1 << modeS); // enable interrupts when enter userspace
	status.spp = modeU;

	frame.status = status.val;
	frame.epc = entry;
	frame.a0 = (addr_t)arg1;
	frame.a1 = (addr_t)arg2;
	frame.ra = threadExitAddr;
	frame.sp = thread->user_stack_base + thread->user_stack_size;
	frame.tp = thread->user_local_storage;

	arch_load_user_iframe(stackHeader, &frame);

	// never return
	return B_ERROR;
}


bool
arch_on_signal_stack(Thread *thread)
{
	struct iframe* frame = thread->arch_info.userFrame;
	if (frame == NULL) {
		panic("arch_on_signal_stack(): No user iframe!");
		return false;
	}

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
	// dprintf("%s(%" B_PRId32 "(%s))\n", __func__, thread->id, thread->name);
	iframe* frame = thread->arch_info.userFrame;

	// fill signal context
	signalFrameData->context.uc_mcontext.x[ 0] = frame->ra;
	signalFrameData->context.uc_mcontext.x[ 1] = frame->sp;
	signalFrameData->context.uc_mcontext.x[ 2] = frame->gp;
	signalFrameData->context.uc_mcontext.x[ 3] = frame->tp;
	signalFrameData->context.uc_mcontext.x[ 4] = frame->t0;
	signalFrameData->context.uc_mcontext.x[ 5] = frame->t1;
	signalFrameData->context.uc_mcontext.x[ 6] = frame->t2;
	signalFrameData->context.uc_mcontext.x[ 7] = frame->fp;
	signalFrameData->context.uc_mcontext.x[ 8] = frame->s1;
	signalFrameData->context.uc_mcontext.x[ 9] = frame->a0;
	signalFrameData->context.uc_mcontext.x[10] = frame->a1;
	signalFrameData->context.uc_mcontext.x[11] = frame->a2;
	signalFrameData->context.uc_mcontext.x[12] = frame->a3;
	signalFrameData->context.uc_mcontext.x[13] = frame->a4;
	signalFrameData->context.uc_mcontext.x[14] = frame->a5;
	signalFrameData->context.uc_mcontext.x[15] = frame->a6;
	signalFrameData->context.uc_mcontext.x[16] = frame->a7;
	signalFrameData->context.uc_mcontext.x[17] = frame->s2;
	signalFrameData->context.uc_mcontext.x[18] = frame->s3;
	signalFrameData->context.uc_mcontext.x[19] = frame->s4;
	signalFrameData->context.uc_mcontext.x[20] = frame->s5;
	signalFrameData->context.uc_mcontext.x[21] = frame->s6;
	signalFrameData->context.uc_mcontext.x[22] = frame->s7;
	signalFrameData->context.uc_mcontext.x[23] = frame->s8;
	signalFrameData->context.uc_mcontext.x[24] = frame->s9;
	signalFrameData->context.uc_mcontext.x[25] = frame->s10;
	signalFrameData->context.uc_mcontext.x[26] = frame->s11;
	signalFrameData->context.uc_mcontext.x[27] = frame->t3;
	signalFrameData->context.uc_mcontext.x[28] = frame->t4;
	signalFrameData->context.uc_mcontext.x[29] = frame->t5;
	signalFrameData->context.uc_mcontext.x[30] = frame->t6;
	signalFrameData->context.uc_mcontext.pc = frame->epc;
	// TODO: don't assume that kernel code don't use FPU
	save_fpu((fpu_context*)&signalFrameData->context.uc_mcontext.f[0]);
	// end of fill signal context

	signal_get_user_stack(frame->sp, &signalFrameData->context.uc_stack);
/*
	dprintf("  thread->signal_stack_enabled: %d\n",
		thread->signal_stack_enabled);
	if (thread->signal_stack_enabled) {
		dprintf("  signal stack: 0x%" B_PRIxADDR " - 0x%" B_PRIxADDR "\n",
			thread->signal_stack_base,
			thread->signal_stack_base + thread->signal_stack_size
		);
	}
*/
	signalFrameData->syscall_restart_return_value = thread->arch_info.oldA0;

	uint8* userStack = get_signal_stack(thread, frame, sa,
		sizeof(*signalFrameData));
	// dprintf("  user stack: 0x%" B_PRIxADDR "\n", (addr_t)userStack);
	status_t res = user_memcpy(userStack, signalFrameData,
		sizeof(*signalFrameData));
	if (res < B_OK)
		return res;

	addr_t commpageAdr = (addr_t)thread->team->commpage_address;
	// dprintf("  commpageAdr: 0x%" B_PRIxADDR "\n", commpageAdr);
	addr_t signalHandlerAddr;
	ASSERT(user_memcpy(&signalHandlerAddr,
		&((addr_t*)commpageAdr)[COMMPAGE_ENTRY_RISCV64_SIGNAL_HANDLER],
		sizeof(signalHandlerAddr)) >= B_OK);
	signalHandlerAddr += commpageAdr;

	frame->ra = frame->epc;
	frame->sp = (addr_t)userStack;
	frame->epc = signalHandlerAddr;
	frame->a0 = frame->sp;

	// WriteTrapInfo();

	return B_OK;
}


int64
arch_restore_signal_frame(struct signal_frame_data* signalFrameData)
{
	// dprintf("arch_restore_signal_frame()\n");
	iframe* frame = thread_get_current_thread()->arch_info.userFrame;

	thread_get_current_thread()->arch_info.oldA0
		= signalFrameData->syscall_restart_return_value;

	frame->ra  = signalFrameData->context.uc_mcontext.x[ 0];
	frame->sp  = signalFrameData->context.uc_mcontext.x[ 1];
	frame->gp  = signalFrameData->context.uc_mcontext.x[ 2];
	frame->tp  = signalFrameData->context.uc_mcontext.x[ 3];
	frame->t0  = signalFrameData->context.uc_mcontext.x[ 4];
	frame->t1  = signalFrameData->context.uc_mcontext.x[ 5];
	frame->t2  = signalFrameData->context.uc_mcontext.x[ 6];
	frame->fp  = signalFrameData->context.uc_mcontext.x[ 7];
	frame->s1  = signalFrameData->context.uc_mcontext.x[ 8];
	frame->a0  = signalFrameData->context.uc_mcontext.x[ 9];
	frame->a1  = signalFrameData->context.uc_mcontext.x[10];
	frame->a2  = signalFrameData->context.uc_mcontext.x[11];
	frame->a3  = signalFrameData->context.uc_mcontext.x[12];
	frame->a4  = signalFrameData->context.uc_mcontext.x[13];
	frame->a5  = signalFrameData->context.uc_mcontext.x[14];
	frame->a6  = signalFrameData->context.uc_mcontext.x[15];
	frame->a7  = signalFrameData->context.uc_mcontext.x[16];
	frame->s2  = signalFrameData->context.uc_mcontext.x[17];
	frame->s3  = signalFrameData->context.uc_mcontext.x[18];
	frame->s4  = signalFrameData->context.uc_mcontext.x[19];
	frame->s5  = signalFrameData->context.uc_mcontext.x[20];
	frame->s6  = signalFrameData->context.uc_mcontext.x[21];
	frame->s7  = signalFrameData->context.uc_mcontext.x[22];
	frame->s8  = signalFrameData->context.uc_mcontext.x[23];
	frame->s9  = signalFrameData->context.uc_mcontext.x[24];
	frame->s10 = signalFrameData->context.uc_mcontext.x[25];
	frame->s11 = signalFrameData->context.uc_mcontext.x[26];
	frame->t3  = signalFrameData->context.uc_mcontext.x[27];
	frame->t4  = signalFrameData->context.uc_mcontext.x[28];
	frame->t5  = signalFrameData->context.uc_mcontext.x[29];
	frame->t6  = signalFrameData->context.uc_mcontext.x[30];
	frame->epc = signalFrameData->context.uc_mcontext.pc;
	restore_fpu((fpu_context*)&signalFrameData->context.uc_mcontext.f[0]);

	return frame->a0;
}


/**	Saves everything needed to restore the frame in the child fork in the
 *	arch_fork_arg structure to be passed to arch_restore_fork_frame().
 *	Also makes sure to return the right value.
 */

void
arch_store_fork_frame(struct arch_fork_arg *arg)
{
/*
	dprintf("arch_store_fork_frame()\n");
	dprintf("  arg: %p\n", arg);
	dprintf("  userFrame: %p\n",
		thread_get_current_thread()->arch_info.userFrame);
*/
	memcpy(&arg->frame, thread_get_current_thread()->arch_info.userFrame,
		sizeof(iframe));
	arg->frame.a0 = 0; // fork return value
}


/** Restores the frame from a forked team as specified by the provided
 *	arch_fork_arg structure.
 *	Needs to be called from within the child team, ie. instead of
 *	arch_thread_enter_uspace() as thread "starter".
 *	This function does not return to the caller, but will enter userland
 *	in the child team at the same position where the parent team left of.
 */

void
arch_restore_fork_frame(struct arch_fork_arg *arg)
{
	//dprintf("arch_restore_fork_frame(%p)\n", arg);
	//dprintf("  thread: %" B_PRId32 "(%s))\n", thread_get_current_thread()->id,
	//	thread_get_current_thread()->name);
	//dprintf("  kernel SP: %#" B_PRIxADDR "\n", thread_get_current_thread()->kernel_stack_top);
	//dprintf("  user PC: "); WritePC(arg->frame.epc); dprintf("\n");

	disable_interrupts();

	arch_stack* stackHeader = (arch_stack*)thread_get_current_thread()->kernel_stack_top - 1;
	stackHeader->thread = thread_get_current_thread();
	SstatusReg status{.val = Sstatus()};
	status.pie = (1 << modeS); // enable interrupts when enter userspace
	status.spp = modeU;
	arg->frame.status = status.val;
	arch_load_user_iframe(stackHeader, &arg->frame);
}
