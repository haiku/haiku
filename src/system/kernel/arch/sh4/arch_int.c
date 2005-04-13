/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <boot/stage2.h>
#include <arch/sh4/vcpu.h>
#include <arch/sh4/sh4.h>

#include <kernel/debug.h>
#include <kernel/int.h>
#include <kernel/thread.h>
#include <kernel/vm_priv.h>
#include <kernel/faults_priv.h>
#include <kernel/syscalls.h>

#include <kernel/arch/sh4/cpu.h>

#define MAX_ARGS 16

struct vector *vector_table;

void arch_int_enable_io_interrupt(int irq)
{
	return;
}

void arch_int_disable_io_interrupt(int irq)
{
	return;
}


static int sh4_handle_exception(void *_frame)
//static int sh4_handle_exception(unsigned int code, unsigned int pc, unsigned int trap, unsigned int page_fault_addr)
{
	struct iframe *frame = (struct iframe *)_frame;
	int ret;

	// NOTE: not safe to do anything that may involve the FPU before
	// it is certain it is not an fpu exception

//	dprintf("sh4_handle_exception: frame 0x%x code 0x%x, pc 0x%x, ssr 0x%x, sr 0x%x, pr 0x%x\n",
//		frame, frame->excode, frame->spc, frame->ssr, get_sr(), frame->pr);
//	dprintf("regs: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
//		frame->r0, frame->r1, frame->r2, frame->r3, frame->r4, frame->r5, frame->r6, frame->r7,
//		frame->r8, frame->r9, frame->r10, frame->r11, frame->r12, frame->r13, frame->r14, frame->sgr);
	switch(frame->excode) {
		case 0:  // reset
		case 1:  // manual reset
		case 5:  // TLB protection violation (read)
		case 6:  // TLB protection violation (write)
		case 7:  // data address error (read)
		case 8:  // data address error (write)
		case 10: // TLB multi hit
		case 12: // illegal instruction
		case 13: // slot illegal instruction
			dprintf("about to gpf at pc 0x%x, excode %d\n", frame->spc, frame->excode);
			ret = general_protection_fault(frame->excode);
			break;
		case 11:  { // TRAPA
			/*
			** arg layout:
			** r4-r7:  arg 1 - 4
			** r0-r3:  arg 5 - 8
			** r8-r13: arg 8 - 13
			*/
			unsigned int *trap = (unsigned int *)0xff000020;
			uint64 retcode;
			unsigned int args[MAX_ARGS];

			int_enable_interrupts();

			thread_atkernel_entry();

			// XXX redo this to be stack-based
			args[0] = frame->r4;
			args[1] = frame->r5;
			args[2] = frame->r6;
			args[3] = frame->r7;
			args[4] = frame->r0;
			args[5] = frame->r1;
			args[6] = frame->r2;
			args[7] = frame->r3;
			args[8] = frame->r8;

			ret = syscall_dispatcher(*trap >> 2, args, &retcode);
			frame->r0 = retcode & 0xffffffff;
			frame->r1 = retcode >> 32;
			break;
		}
		case 9: { // FPU exception
			int fpu_fault_code;
			switch(frame->fpscr & 0x0003f000) {
				case 0x1000:
					fpu_fault_code = FPU_FAULT_CODE_INEXACT;
					break;
				case 0x2000:
					fpu_fault_code = FPU_FAULT_CODE_UNDERFLOW;
					break;
				case 0x4000:
					fpu_fault_code = FPU_FAULT_CODE_OVERFLOW;
					break;
				case 0x8000:
					fpu_fault_code = FPU_FAULT_CODE_DIVBYZERO;
					break;
				case 0x10000:
					fpu_fault_code = FPU_FAULT_CODE_INVALID_OP;
					break;
				case 0x20000:
					fpu_fault_code = FPU_FAULT_CODE_UNKNOWN;
					break;
				default:
					// XXX handle better
					fpu_fault_code = FPU_FAULT_CODE_UNKNOWN;
			}
			ret = fpu_fault(fpu_fault_code);
			break;
		}
		case 64: // FPU disable exception
		case 65: // Slot FPU disable exception
			ret = fpu_disable_fault();
			break;
		case EXCEPTION_PAGE_FAULT_READ:
		case EXCEPTION_PAGE_FAULT_WRITE: {
			addr newip;
			if((frame->ssr & 0x000000f0) == 0) {
//				dprintf("page_fault: enabling interrupts\n");
				int_enable_interrupts();
			}
			ret = vm_page_fault(frame->page_fault_addr, frame->spc,
				frame->excode == EXCEPTION_PAGE_FAULT_WRITE, (frame->ssr & 0x40000000) == 0, &newip);
			if(newip != 0)
				frame->spc = newip;
			break;
		}
		default:
			ret = int_io_interrupt_handler(frame->excode);
	}
	if(ret == INT_RESCHEDULE) {
		int state = int_disable_interrupts();
		GRAB_THREAD_LOCK();
		thread_resched();
		RELEASE_THREAD_LOCK();
		int_restore_interrupts(state);
	}
	if(!(frame->ssr & 0x40000000) || (frame->excode == 11)) {
		thread_atkernel_exit();
	}

//	dprintf("sh4_handle_exception: exit\n");

	return 0;
}

int arch_int_init(kernel_args *ka)
{
	int i;

	dprintf("arch_int_init: entry\n");

	vector_table = (struct vector *)ka->arch_args.vcpu->vt;
	dprintf("arch_int_init: vector table 0x%x\n", vector_table);

	// set up the vectors
	// handle all of them
	for(i=0; i<256; i++)
		vector_table[i].func = &sh4_handle_exception;

	return 0;
}

int arch_int_init2(kernel_args *ka)
{
	return 0;
}

