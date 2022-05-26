/*
 * Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk.
 * Distributed under the terms of the MIT License.
 */


#include <debugger.h>
#include <int.h>
#include <thread.h>
#include <arch/user_debugger.h>


void
arch_clear_team_debug_info(struct arch_team_debug_info *info)
{
}


void
arch_destroy_team_debug_info(struct arch_team_debug_info *info)
{
	arch_clear_team_debug_info(info);
}


void
arch_clear_thread_debug_info(struct arch_thread_debug_info *info)
{
}


void
arch_destroy_thread_debug_info(struct arch_thread_debug_info *info)
{
	arch_clear_thread_debug_info(info);
}


void
arch_update_thread_single_step()
{
}


void
arch_set_debug_cpu_state(const debug_cpu_state *cpuState)
{
	iframe* frame = thread_get_current_thread()->arch_info.userFrame;

	frame->ra  = cpuState->x[ 0];
	frame->sp  = cpuState->x[ 1];
	frame->gp  = cpuState->x[ 2];
	frame->tp  = cpuState->x[ 3];
	frame->t0  = cpuState->x[ 4];
	frame->t1  = cpuState->x[ 5];
	frame->t2  = cpuState->x[ 6];
	frame->fp  = cpuState->x[ 7];
	frame->s1  = cpuState->x[ 8];
	frame->a0  = cpuState->x[ 9];
	frame->a1  = cpuState->x[10];
	frame->a2  = cpuState->x[11];
	frame->a3  = cpuState->x[12];
	frame->a4  = cpuState->x[13];
	frame->a5  = cpuState->x[14];
	frame->a6  = cpuState->x[15];
	frame->a7  = cpuState->x[16];
	frame->s2  = cpuState->x[17];
	frame->s3  = cpuState->x[18];
	frame->s4  = cpuState->x[19];
	frame->s5  = cpuState->x[20];
	frame->s6  = cpuState->x[21];
	frame->s7  = cpuState->x[22];
	frame->s8  = cpuState->x[23];
	frame->s9  = cpuState->x[24];
	frame->s10 = cpuState->x[25];
	frame->s11 = cpuState->x[26];
	frame->t3  = cpuState->x[27];
	frame->t4  = cpuState->x[28];
	frame->t5  = cpuState->x[29];
	frame->t6  = cpuState->x[30];
	frame->epc = cpuState->pc;
	restore_fpu((fpu_context*)&cpuState->f[0]);
}


void
arch_get_debug_cpu_state(debug_cpu_state *cpuState)
{
	arch_get_thread_debug_cpu_state(thread_get_current_thread(), cpuState);
}


status_t
arch_get_thread_debug_cpu_state(Thread* thread, debug_cpu_state* cpuState)
{
	iframe* frame = thread->arch_info.userFrame;
	if (frame == NULL)
		return B_BAD_DATA;

	cpuState->x[ 0] = frame->ra;
	cpuState->x[ 1] = frame->sp;
	cpuState->x[ 2] = frame->gp;
	cpuState->x[ 3] = frame->tp;
	cpuState->x[ 4] = frame->t0;
	cpuState->x[ 5] = frame->t1;
	cpuState->x[ 6] = frame->t2;
	cpuState->x[ 7] = frame->fp;
	cpuState->x[ 8] = frame->s1;
	cpuState->x[ 9] = frame->a0;
	cpuState->x[10] = frame->a1;
	cpuState->x[11] = frame->a2;
	cpuState->x[12] = frame->a3;
	cpuState->x[13] = frame->a4;
	cpuState->x[14] = frame->a5;
	cpuState->x[15] = frame->a6;
	cpuState->x[16] = frame->a7;
	cpuState->x[17] = frame->s2;
	cpuState->x[18] = frame->s3;
	cpuState->x[19] = frame->s4;
	cpuState->x[20] = frame->s5;
	cpuState->x[21] = frame->s6;
	cpuState->x[22] = frame->s7;
	cpuState->x[23] = frame->s8;
	cpuState->x[24] = frame->s9;
	cpuState->x[25] = frame->s10;
	cpuState->x[26] = frame->s11;
	cpuState->x[27] = frame->t3;
	cpuState->x[28] = frame->t4;
	cpuState->x[29] = frame->t5;
	cpuState->x[30] = frame->t6;
	cpuState->pc = frame->epc;
	// TODO: don't assume that kernel code don't use FPU
	if (thread == thread_get_current_thread())
		save_fpu((fpu_context*)&cpuState->f[0]);
	else
		memcpy(&cpuState->f[0], &thread->arch_info.fpuContext,
			sizeof(thread->arch_info.fpuContext));

	return B_OK;
}


status_t
arch_set_breakpoint(void *address)
{
	return B_ERROR;
}


status_t
arch_clear_breakpoint(void *address)
{
	return B_ERROR;
}


status_t
arch_set_watchpoint(void *address, uint32 type, int32 length)
{
	return B_ERROR;
}


status_t
arch_clear_watchpoint(void *address)
{
	return B_ERROR;
}

bool
arch_has_breakpoints(struct arch_team_debug_info *info)
{
	return false;
}
