/*
 * Copyright 2003-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include <int.h>
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


//#pragma mark debug output

static void
WriteMode(int mode)
{
	switch (mode) {
		case modeU: dprintf("u"); break;
		case modeS: dprintf("s"); break;
		case modeM: dprintf("m"); break;
		default: dprintf("%d", mode);
	}
}


static void
WriteModeSet(uint32_t val)
{
	bool first = true;
	dprintf("{");
	for (int i = 0; i < 32; i++) {
		if (((1LL << i) & val) != 0) {
			if (first) first = false; else dprintf(", ");
			WriteMode(i);
		}
	}
	dprintf("}");
}


static void
WriteExt(uint64_t val)
{
	switch (val) {
		case 0: dprintf("off"); break;
		case 1: dprintf("initial"); break;
		case 2: dprintf("clean"); break;
		case 3: dprintf("dirty"); break;
		default: dprintf("%" B_PRId64, val);
	}
}


static void
WriteSstatus(uint64_t val)
{
	SstatusReg status{.val = val};
	dprintf("(");
	dprintf("ie: "); WriteModeSet(status.ie);
	dprintf(", pie: "); WriteModeSet(status.pie);
	dprintf(", spp: "); WriteMode(status.spp);
	dprintf(", fs: "); WriteExt(status.fs);
	dprintf(", xs: "); WriteExt(status.xs);
	dprintf(", sum: %d", (int)status.sum);
	dprintf(", mxr: %d", (int)status.mxr);
	dprintf(", uxl: %d", (int)status.uxl);
	dprintf(", sd: %d", (int)status.sd);
	dprintf(")");
}


static void
WriteInterrupt(uint64_t val)
{
	switch (val) {
		case 0 + modeU: dprintf("uSoft"); break;
		case 0 + modeS: dprintf("sSoft"); break;
		case 0 + modeM: dprintf("mSoft"); break;
		case 4 + modeU: dprintf("uTimer"); break;
		case 4 + modeS: dprintf("sTimer"); break;
		case 4 + modeM: dprintf("mTimer"); break;
		case 8 + modeU: dprintf("uExtern"); break;
		case 8 + modeS: dprintf("sExtern"); break;
		case 8 + modeM: dprintf("mExtern"); break;
		default: dprintf("%" B_PRId64, val);
	}
}


static void
WriteInterruptSet(uint64_t val)
{
	bool first = true;
	dprintf("{");
	for (int i = 0; i < 64; i++) {
		if (((1LL << i) & val) != 0) {
			if (first) first = false; else dprintf(", ");
			WriteInterrupt(i);
		}
	}
	dprintf("}");
}


static void
WriteCause(uint64_t cause)
{
	if ((cause & causeInterrupt) == 0) {
		dprintf("exception ");
		switch (cause) {
			case causeExecMisalign: dprintf("execMisalign"); break;
			case causeExecAccessFault: dprintf("execAccessFault"); break;
			case causeIllegalInst: dprintf("illegalInst"); break;
			case causeBreakpoint: dprintf("breakpoint"); break;
			case causeLoadMisalign: dprintf("loadMisalign"); break;
			case causeLoadAccessFault: dprintf("loadAccessFault"); break;
			case causeStoreMisalign: dprintf("storeMisalign"); break;
			case causeStoreAccessFault: dprintf("storeAccessFault"); break;
			case causeUEcall: dprintf("uEcall"); break;
			case causeSEcall: dprintf("sEcall"); break;
			case causeMEcall: dprintf("mEcall"); break;
			case causeExecPageFault: dprintf("execPageFault"); break;
			case causeLoadPageFault: dprintf("loadPageFault"); break;
			case causeStorePageFault: dprintf("storePageFault"); break;
			default: dprintf("%" B_PRId64, cause);
			}
	} else {
		dprintf("interrupt "); WriteInterrupt(cause & ~causeInterrupt);
	}
}


const static char* registerNames[] = {
	" ra", " t6", " sp", " gp",
	" tp", " t0", " t1", " t2",
	" t5", " s1", " a0", " a1",
	" a2", " a3", " a4", " a5",
	" a6", " a7", " s2", " s3",
	" s4", " s5", " s6", " s7",
	" s8", " s9", "s10", "s11",
	" t3", " t4", " fp", "epc"
};


static void WriteRegisters(iframe* frame)
{
	uint64* regs = &frame->ra;
	for (int i = 0; i < 32; i += 4) {
		dprintf(
			"  %s: 0x%016" B_PRIx64
			"  %s: 0x%016" B_PRIx64
			"  %s: 0x%016" B_PRIx64
			"  %s: 0x%016" B_PRIx64 "\n",
			registerNames[i + 0], regs[i + 0],
			registerNames[i + 1], regs[i + 1],
			registerNames[i + 2], regs[i + 2],
			registerNames[i + 3], regs[i + 3]
		);
	}
}


static void
DumpMemory(uint64* adr, size_t len)
{
	while (len > 0) {
		if ((addr_t)adr % 0x10 == 0)
			dprintf("%08" B_PRIxADDR " ", (addr_t)adr);
		uint64 val;
		if (user_memcpy(&val, adr++, sizeof(val)) < B_OK) {
			dprintf(" ????????????????");
		} else {
			dprintf(" %016" B_PRIx64, val);
		}
		if ((addr_t)adr % 0x10 == 0)
			dprintf("\n");
		len -= 8;
	}
	if ((addr_t)adr % 0x10 != 0)
		dprintf("\n");

	dprintf("%08" B_PRIxADDR "\n\n", (addr_t)adr);
}


void
WriteTrapInfo(iframe* frame)
{
	InterruptsLocker locker;
	dprintf("STrap("); WriteCause(frame->cause); dprintf(")\n");
	dprintf("  sstatus: "); WriteSstatus(frame->status); dprintf("\n");
//	dprintf("  sie: "); WriteInterruptSet(Sie()); dprintf("\n");
//	dprintf("  sip: "); WriteInterruptSet(Sip()); dprintf("\n");
	//dprintf("  stval: "); WritePC(Stval()); dprintf("\n");
	dprintf("  stval: 0x%" B_PRIx64 "\n", frame->tval);
//	dprintf("  tp: 0x%" B_PRIxADDR "(%s)\n", Tp(),
//		thread_get_current_thread()->name);

	WriteRegisters(frame);
#if 0
	dprintf("  kernel stack: %#" B_PRIxADDR " - %#" B_PRIxADDR "\n",
		thread_get_current_thread()->kernel_stack_base,
		thread_get_current_thread()->kernel_stack_top - 1
	);
	dprintf("  user stack: %#" B_PRIxADDR " - %#" B_PRIxADDR "\n",
		thread_get_current_thread()->user_stack_base,
		thread_get_current_thread()->user_stack_base +
		thread_get_current_thread()->user_stack_size - 1
	);
	if (thread_get_current_thread()->arch_info.userFrame != NULL) {
		WriteRegisters(thread_get_current_thread()->arch_info.userFrame);

		dprintf("Stack memory dump:\n");
		DumpMemory(
			(uint64*)thread_get_current_thread()->arch_info.userFrame->sp,
			thread_get_current_thread()->user_stack_base +
			thread_get_current_thread()->user_stack_size -
			thread_get_current_thread()->arch_info.userFrame->sp
		);
//		if (true) {
//		} else {
//			DumpMemory((uint64*)frame->sp, thread_get_current_thread()->kernel_stack_top - frame->sp);
//		}
	}
#endif
}


//#pragma mark -

static void
SendSignal(debug_exception_type type, uint32 signalNumber, int32 signalCode,
	addr_t signalAddress = 0, int32 signalError = B_ERROR)
{
	if (SstatusReg{.val = Sstatus()}.spp == modeU) {
		struct sigaction action;
		Thread* thread = thread_get_current_thread();

		DoStackTrace(Fp(), 0);

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
	if (thread->cpu->invoke_scheduler) {
		SpinLocker schedulerLocker(thread->scheduler_lock);
		scheduler_reschedule(B_THREAD_READY);
		schedulerLocker.Unlock();
		restore_interrupts(state);
	} else if (thread->post_interrupt_callback != NULL) {
		void (*callback)(void*) = thread->post_interrupt_callback;
		void* data = thread->post_interrupt_data;

		thread->post_interrupt_callback = NULL;
		thread->post_interrupt_data = NULL;

		restore_interrupts(state);

		callback(data);
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
/*
			dprintf("SetAccessedFlags(%#" B_PRIxADDR ", %d)\n", addr, isWrite);
*/
			return true;
		}
	} else {
		if (
			((B_READ_AREA | B_KERNEL_READ_AREA) & pageFlags) != 0
			&& (PAGE_ACCESSED & pageFlags) == 0
		) {
			map->SetFlags(addr, PAGE_ACCESSED);
/*
			dprintf("SetAccessedFlags(%#" B_PRIxADDR ", %d)\n", addr, isWrite);
*/
			return true;
		}
	}
	return false;
}


extern "C" void
STrap(iframe* frame)
{
	// dprintf("STrap("); WriteCause(Scause()); dprintf(")\n");

/*
	iframe oldFrame = *frame;
	const auto& frameChangeChecker = MakeScopeExit([&]() {
			InterruptsLocker locker;
			bool first = true;
			for (int i = 0; i < 32; i++) {
				uint64 oldVal = ((int64*)&oldFrame)[i];
				uint64 newVal = ((int64*)frame)[i];
				if (oldVal != newVal) {
					if (first) {
						dprintf("FrameChangeChecker, thread: %" B_PRId32 "(%s)\n", thread_get_current_thread()->id, thread_get_current_thread()->name);
						first = false;
					}
					dprintf("  %s: %#" B_PRIxADDR " -> %#" B_PRIxADDR "\n", registerNames[i], oldVal, newVal);
				}
			}

			if (frame->epc == 0)
				panic("FrameChangeChecker: EPC = 0");
	});
*/
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
			SetSip(Sip() & ~(1 << sSoftInt));
			// dprintf("sSoftInt(%" B_PRId32 ")\n", smp_get_current_cpu());
			smp_intercpu_int_handler(smp_get_current_cpu());
			AfterInterrupt();
			return;
		}
		case causeInterrupt + sTimerInt: {
			// SetSie(Sie() & ~(1 << sTimerInt));
			// dprintf("sTimerInt(%" B_PRId32 ")\n", smp_get_current_cpu());
			timer_interrupt();
			AfterInterrupt();
			return;
		}
		case causeInterrupt + sExternInt: {
			uint64 irq = gPlicRegs->contexts[sPlicContexts[smp_get_current_cpu()]].claimAndComplete;
			int_io_interrupt_handler(irq, true);
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
/*
			switch (syscall) {
				case SYSCALL_READ_PORT_ETC:
				case SYSCALL_WRITE_PORT_ETC:
					DoStackTrace(Fp(), 0);
					break;
			}
*/
			// dprintf("syscall: %s\n", kExtendedSyscallInfos[syscall].name);

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
arch_int_enable_io_interrupt(int irq)
{
	dprintf("arch_int_enable_io_interrupt(%d)\n", irq);
	gPlicRegs->priority[irq] = 1;
	gPlicRegs->enable[sPlicContexts[0]][irq / 32] |= 1 << (irq % 32);
}


void
arch_int_disable_io_interrupt(int irq)
{
	dprintf("arch_int_disable_io_interrupt(%d)\n", irq);
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
