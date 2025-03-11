/*
 * Copyright 2003-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include <interrupts.h>
#include <cpu.h>
#include <thread.h>
#include <vm/vm_priv.h>
#include <ksyscalls.h>
#include <syscall_numbers.h>
#include <arch_cpu_defs.h>
#include <arch_thread_types.h>
#include <arch/debug.h>
#include <util/AutoLock.h>
#include <Htif.h>
#include <Plic.h>
#include <Clint.h>
#include <AutoDeleterDrivers.h>
#include <ScopeExit.h>
#include "RISCV64VMTranslationMap.h"

#include <algorithm>


static uint32 sPlicContexts[SMP_MAX_CPUS];


//#pragma mark -

static void
SendSignal(debug_exception_type type, uint32 signalNumber, int32 signalCode,
	addr_t signalAddress = 0, int32 signalError = B_ERROR)
{
	if (SstatusReg{.val = Sstatus()}.spp == modeU) {
		struct sigaction action;
		Thread* thread = thread_get_current_thread();

		enable_interrupts();

		// If the thread has a signal handler for the signal, we simply send it
		// the signal. Otherwise we notify the user debugger first.
		if ((sigaction(signalNumber, NULL, &action) == 0
				&& action.sa_handler != SIG_DFL
				&& action.sa_handler != SIG_IGN)
			|| user_debug_exception_occurred(type, signalNumber)) {
			Signal signal(signalNumber, signalCode, signalError,
				thread->team->id);
			signal.SetAddress((void*)signalAddress);
			send_signal_to_thread(thread, signal, 0);
		}
	} else {
		panic("Unexpected exception occurred in kernel mode!");
	}
}


static void
AfterInterrupt()
{
	if (debug_debugger_running())
		return;

	Thread* thread = thread_get_current_thread();
	cpu_status state = disable_interrupts();
	if (thread->post_interrupt_callback != NULL) {
		void (*callback)(void*) = thread->post_interrupt_callback;
		void* data = thread->post_interrupt_data;

		thread->post_interrupt_callback = NULL;
		thread->post_interrupt_data = NULL;

		restore_interrupts(state);

		callback(data);
	} else if (thread->cpu->invoke_scheduler) {
		SpinLocker schedulerLocker(thread->scheduler_lock);
		scheduler_reschedule(B_THREAD_READY);
		schedulerLocker.Unlock();
		restore_interrupts(state);
	}
}


static bool
SetAccessedFlags(addr_t addr, bool isWrite)
{
	VMAddressSpacePutter addressSpace;
	if (IS_KERNEL_ADDRESS(addr))
		addressSpace.SetTo(VMAddressSpace::GetKernel());
	else if (IS_USER_ADDRESS(addr))
		addressSpace.SetTo(VMAddressSpace::GetCurrent());

	if(!addressSpace.IsSet())
		return false;

	RISCV64VMTranslationMap* map
		= (RISCV64VMTranslationMap*)addressSpace->TranslationMap();

	phys_addr_t physAdr;
	uint32 pageFlags;
	map->QueryInterrupt(addr, &physAdr, &pageFlags);

	if ((PAGE_PRESENT & pageFlags) == 0)
		return false;

	if (isWrite) {
		if (
			((B_WRITE_AREA | B_KERNEL_WRITE_AREA) & pageFlags) != 0
			&& ((PAGE_ACCESSED | PAGE_MODIFIED) & pageFlags)
				!= (PAGE_ACCESSED | PAGE_MODIFIED)
		) {
			map->SetFlags(addr, PAGE_ACCESSED | PAGE_MODIFIED);
			return true;
		}
	} else {
		if (
			((B_READ_AREA | B_KERNEL_READ_AREA) & pageFlags) != 0
			&& (PAGE_ACCESSED & pageFlags) == 0
		) {
			map->SetFlags(addr, PAGE_ACCESSED);
			return true;
		}
	}
	return false;
}


extern "C" void
STrap(iframe* frame)
{
	switch (frame->cause) {
		case causeExecPageFault:
		case causeLoadPageFault:
		case causeStorePageFault: {
			if (SetAccessedFlags(Stval(), frame->cause == causeStorePageFault))
				return;
		}
	}

	if (SstatusReg{.val = frame->status}.spp == modeU) {
		thread_get_current_thread()->arch_info.userFrame = frame;
		thread_get_current_thread()->arch_info.oldA0 = frame->a0;
		thread_at_kernel_entry(system_time());
	}
	const auto& kernelExit = ScopeExit([&]() {
		if (SstatusReg{.val = frame->status}.spp == modeU) {
			disable_interrupts();
			atomic_and(&thread_get_current_thread()->flags, ~THREAD_FLAGS_SYSCALL_RESTARTED);
			if ((thread_get_current_thread()->flags
				& (THREAD_FLAGS_SIGNALS_PENDING
				| THREAD_FLAGS_DEBUG_THREAD
				| THREAD_FLAGS_TRAP_FOR_CORE_DUMP)) != 0) {
				enable_interrupts();
				thread_at_kernel_exit();
			} else {
				thread_at_kernel_exit_no_signals();
			}
			if ((THREAD_FLAGS_RESTART_SYSCALL & thread_get_current_thread()->flags) != 0) {
				atomic_and(&thread_get_current_thread()->flags, ~THREAD_FLAGS_RESTART_SYSCALL);
				atomic_or(&thread_get_current_thread()->flags, THREAD_FLAGS_SYSCALL_RESTARTED);

				frame->a0 = thread_get_current_thread()->arch_info.oldA0;
				frame->epc -= 4;
			}
			thread_get_current_thread()->arch_info.userFrame = NULL;
		}
	});

	switch (frame->cause) {
		case causeIllegalInst: {
			return SendSignal(B_INVALID_OPCODE_EXCEPTION, SIGILL, ILL_ILLOPC,
				frame->epc);
		}
		case causeExecMisalign:
		case causeLoadMisalign:
		case causeStoreMisalign: {
			return SendSignal(B_ALIGNMENT_EXCEPTION, SIGBUS, BUS_ADRALN,
				Stval());
		}
		case causeBreakpoint: {
			if (SstatusReg{.val = frame->status}.spp == modeU) {
				user_debug_breakpoint_hit(false);
			} else {
				panic("hit kernel breakpoint");
			}
			return;
		}
		case causeExecAccessFault:
		case causeLoadAccessFault:
		case causeStoreAccessFault: {
			return SendSignal(B_SEGMENT_VIOLATION, SIGBUS, BUS_ADRERR,
				Stval());
		}
		case causeExecPageFault:
		case causeLoadPageFault:
		case causeStorePageFault: {
			uint64 stval = Stval();

			if (debug_debugger_running()) {
				Thread* thread = thread_get_current_thread();
				if (thread != NULL) {
					cpu_ent* cpu = &gCPU[smp_get_current_cpu()];
					if (cpu->fault_handler != 0) {
						debug_set_page_fault_info(stval, frame->epc,
							(frame->cause == causeStorePageFault)
								? DEBUG_PAGE_FAULT_WRITE : 0);
						frame->epc = cpu->fault_handler;
						frame->sp = cpu->fault_handler_stack_pointer;
						return;
					}

					if (thread->fault_handler != 0) {
						kprintf("ERROR: thread::fault_handler used in kernel "
							"debugger!\n");
						debug_set_page_fault_info(stval, frame->epc,
							frame->cause == causeStorePageFault
								? DEBUG_PAGE_FAULT_WRITE : 0);
						frame->epc = (addr_t)thread->fault_handler;
						return;
					}
				}

				panic("page fault in debugger without fault handler! Touching "
					"address %p from ip %p\n", (void*)stval, (void*)frame->epc);
				return;
			}

			if (SstatusReg{.val = frame->status}.pie == 0) {
				// user_memcpy() failure
				Thread* thread = thread_get_current_thread();
				if (thread != NULL && thread->fault_handler != 0) {
					addr_t handler = (addr_t)(thread->fault_handler);
					if (frame->epc != handler) {
						frame->epc = handler;
						return;
					}
				}
				panic("page fault with interrupts disabled@!dump_virt_page %#" B_PRIx64, stval);
			}

			addr_t newIP = 0;
			enable_interrupts();

			vm_page_fault(stval, frame->epc, frame->cause == causeStorePageFault,
				frame->cause == causeExecPageFault,
				SstatusReg{.val = frame->status}.spp == modeU, &newIP);

			if (newIP != 0)
				frame->epc = newIP;

			return;
		}
		case causeInterrupt + sSoftInt: {
			ClearBitsSip(1 << sSoftInt);
			// dprintf("sSoftInt(%" B_PRId32 ")\n", smp_get_current_cpu());
			smp_intercpu_interrupt_handler(smp_get_current_cpu());
			AfterInterrupt();
			return;
		}
		case causeInterrupt + sTimerInt: {
			ClearBitsSie(1 << sTimerInt);
			// dprintf("sTimerInt(%" B_PRId32 ")\n", smp_get_current_cpu());
			timer_interrupt();
			AfterInterrupt();
			return;
		}
		case causeInterrupt + sExternInt: {
			uint64 irq = gPlicRegs->contexts[sPlicContexts[smp_get_current_cpu()]].claimAndComplete;
			io_interrupt_handler(irq, true);
			gPlicRegs->contexts[sPlicContexts[smp_get_current_cpu()]].claimAndComplete = irq;
			AfterInterrupt();
			return;
		}
		case causeUEcall: {
			frame->epc += 4; // skip ecall
			uint64 syscall = frame->t0;
			uint64 args[20];
			if (syscall < (uint64)kSyscallCount) {
				uint32 argCnt = kExtendedSyscallInfos[syscall].parameter_count;
				memcpy(&args[0], &frame->a0,
					sizeof(uint64)*std::min<uint32>(argCnt, 8));
				if (argCnt > 8) {
					if (status_t res = user_memcpy(&args[8], (void*)frame->sp,
						sizeof(uint64)*(argCnt - 8)) < B_OK) {
						dprintf("can't read syscall arguments on user "
							"stack\n");
						frame->a0 = res;
						return;
					}
				}
			}

			enable_interrupts();
			uint64 returnValue = 0;
			syscall_dispatcher(syscall, (void*)args, &returnValue);
			frame->a0 = returnValue;
			return;
		}
	}
	panic("unhandled STrap");
}


//#pragma mark -

status_t
arch_int_init(kernel_args* args)
{
	dprintf("arch_int_init()\n");

	for (uint32 i = 0; i < args->num_cpus; i++) {
		dprintf("  CPU %" B_PRIu32 ":\n", i);
		dprintf("    hartId: %" B_PRIu32 "\n", args->arch_args.hartIds[i]);
		dprintf("    plicContext: %" B_PRIu32 "\n", args->arch_args.plicContexts[i]);
	}

	for (uint32 i = 0; i < args->num_cpus; i++)
		sPlicContexts[i] = args->arch_args.plicContexts[i];

	// TODO: read from FDT
	reserve_io_interrupt_vectors(128, 0, INTERRUPT_TYPE_IRQ);

	for (uint32 i = 0; i < args->num_cpus; i++)
		gPlicRegs->contexts[sPlicContexts[i]].priorityThreshold = 0;

	return B_OK;
}


status_t
arch_int_init_post_vm(kernel_args* args)
{
	return B_OK;
}


status_t
arch_int_init_post_device_manager(struct kernel_args* args)
{
	return B_OK;
}


status_t
arch_int_init_io(kernel_args* args)
{
	return B_OK;
}


void
arch_int_enable_io_interrupt(int32 irq)
{
	dprintf("arch_int_enable_io_interrupt(%" B_PRId32 ")\n", irq);
	gPlicRegs->priority[irq] = 1;
	gPlicRegs->enable[sPlicContexts[0]][irq / 32] |= 1 << (irq % 32);
}


void
arch_int_disable_io_interrupt(int32 irq)
{
	dprintf("arch_int_disable_io_interrupt(%" B_PRId32 ")\n", irq);
	gPlicRegs->priority[irq] = 0;
	gPlicRegs->enable[sPlicContexts[0]][irq / 32] &= ~(1 << (irq % 32));
}


int32
arch_int_assign_to_cpu(int32 irq, int32 cpu)
{
	// Not yet supported.
	return 0;
}


#undef arch_int_enable_interrupts
#undef arch_int_disable_interrupts
#undef arch_int_restore_interrupts
#undef arch_int_are_interrupts_enabled


extern "C" void
arch_int_enable_interrupts()
{
	arch_int_enable_interrupts_inline();
}


extern "C" int
arch_int_disable_interrupts()
{
	return arch_int_disable_interrupts_inline();
}


extern "C" void
arch_int_restore_interrupts(int oldState)
{
	arch_int_restore_interrupts_inline(oldState);
}


extern "C" bool
arch_int_are_interrupts_enabled()
{
	return arch_int_are_interrupts_enabled_inline();
}
