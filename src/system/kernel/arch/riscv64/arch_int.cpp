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
#include "RISCV64VMTranslationMap.h"

#include <algorithm>


static uint32 sBootHartId = 0;
static int32 sPlicContextOfs = 0;


extern "C" void SVec();
extern "C" void SVecU();


//#pragma mark debug output

void
WriteMode(int mode)
{
	switch (mode) {
		case modeU: dprintf("u"); break;
		case modeS: dprintf("s"); break;
		case modeM: dprintf("m"); break;
		default: dprintf("%d", mode);
	}
}


void
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


void
WriteMstatus(uint64_t val)
{
	MstatusReg status(val);
	dprintf("(");
	dprintf("ie: "); WriteModeSet(status.ie);
	dprintf(", pie: "); WriteModeSet(status.pie);
	dprintf(", spp: "); WriteMode(status.spp);
	dprintf(", mpp: "); WriteMode(status.mpp);
	dprintf(", sum: %d", (int)status.sum);
	dprintf(")");
}


void
WriteSstatus(uint64_t val)
{
	SstatusReg status(val);
	dprintf("(");
	dprintf("ie: "); WriteModeSet(status.ie);
	dprintf(", pie: "); WriteModeSet(status.pie);
	dprintf(", spp: "); WriteMode(status.spp);
	dprintf(", sum: %d", (int)status.sum);
	dprintf(")");
}


void
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


void
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


void
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


void
WriteTrapInfo()
{
	InterruptsLocker locker;
	dprintf("STrap("); WriteCause(Scause()); dprintf(")\n");
	dprintf("  sstatus: "); WriteSstatus(Sstatus()); dprintf("\n");
	dprintf("  sie: "); WriteInterruptSet(Sie()); dprintf("\n");
	dprintf("  sip: "); WriteInterruptSet(Sip()); dprintf("\n");
	//dprintf("  stval: "); WritePC(Stval()); dprintf("\n");
	dprintf("  stval: 0x%" B_PRIx64 "\n", Stval());
	dprintf("  tp: 0x%" B_PRIxADDR "(%s)\n", Tp(),
		thread_get_current_thread()->name);
}


//#pragma mark -

static void
SendSignal(debug_exception_type type, uint32 signalNumber, int32 signalCode,
	addr_t signalAddress = 0, int32 signalError = B_ERROR)
{
	if (SstatusReg(Sstatus()).spp == modeU) {
		struct sigaction action;
		Thread* thread = thread_get_current_thread();

		WriteTrapInfo();
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
		WriteTrapInfo();
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


static void
WriteProtection(uint32 flags)
{
	dprintf("kernel: {");
	if (B_KERNEL_READ_AREA & flags) dprintf("R");
	if (B_KERNEL_WRITE_AREA & flags) dprintf("W");
	if (B_KERNEL_EXECUTE_AREA & flags) dprintf("X");
	if (B_KERNEL_STACK_AREA & flags) dprintf("S");
	dprintf("}, user: {");
	if (B_READ_AREA & flags) dprintf("R");
	if (B_WRITE_AREA & flags) dprintf("W");
	if (B_EXECUTE_AREA & flags) dprintf("X");
	if (B_STACK_AREA & flags) dprintf("S");
	dprintf("}");
}


// TODO: needs moved into an arch-agnostic location?

template<typename F>
class ScopeExit 
{
public:
	explicit ScopeExit(F&& fn) : fFn(fn)
	{
	}

	~ScopeExit()
	{
		fFn();
	}

	ScopeExit(ScopeExit&& other) : fFn(std::move(other.fFn))
	{
	}

private:
	ScopeExit(const ScopeExit&);
	ScopeExit& operator=(const ScopeExit&);

private:
	F fFn;
};

template<typename F>
ScopeExit<F> MakeScopeExit(F&& fn)
{
	return ScopeExit<F>(std::move(fn));
}


extern "C" void
STrap(iframe* frame)
{
	// dprintf("STrap("); WriteCause(Scause()); dprintf(")\n");

	SstatusReg status(Sstatus());
	uint64 cause = Scause();

	const auto& statusRestorer = MakeScopeExit([&]() {
		SetSstatus(status.val);
	});

	switch (cause) {
		case causeExecPageFault:
		case causeLoadPageFault:
		case causeStorePageFault: {
			if (SetAccessedFlags(Stval(), cause == causeStorePageFault))
				return;
		}
	}

	if (status.spp == modeU) {
		thread_get_current_thread()->arch_info.userFrame = frame;
		thread_at_kernel_entry(system_time());
	}
	const auto& kernelExit = MakeScopeExit([&]() {
		if (status.spp == modeU) {
			disable_interrupts();
			if ((thread_get_current_thread()->flags
				& (THREAD_FLAGS_SIGNALS_PENDING
				| THREAD_FLAGS_DEBUG_THREAD
				| THREAD_FLAGS_TRAP_FOR_CORE_DUMP)) != 0) {
				enable_interrupts();
				thread_at_kernel_exit();
			} else {
				thread_at_kernel_exit_no_signals();
			}
			thread_get_current_thread()->arch_info.userFrame = NULL;
		}
	});

	switch (cause) {
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
		// case causeBreakpoint:
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
							(cause == causeStorePageFault)
								? DEBUG_PAGE_FAULT_WRITE : 0);
						frame->epc = cpu->fault_handler;
						frame->sp = cpu->fault_handler_stack_pointer;
						return;
					}

					if (thread->fault_handler != 0) {
						kprintf("ERROR: thread::fault_handler used in kernel "
							"debugger!\n");
						debug_set_page_fault_info(stval, frame->epc,
							cause == causeStorePageFault
								? DEBUG_PAGE_FAULT_WRITE : 0);
						frame->epc = (addr_t)thread->fault_handler;
						return;
					}
				}

				panic("page fault in debugger without fault handler! Touching "
					"address %p from ip %p\n", (void*)stval, (void*)frame->epc);
				return;
			}

			if (status.pie == 0) {
				WriteTrapInfo();
				panic("page fault with interrupts disabled@!dump_virt_page %#" B_PRIx64, stval);
			}

			addr_t newIP = 0;
			enable_interrupts();
			vm_page_fault(stval, frame->epc, cause == causeStorePageFault,
				cause == causeExecPageFault, status.spp == modeU, status.sum != 0, &newIP);
			if (newIP != 0)
				frame->epc = newIP;

			return;
		}
		case causeInterrupt + sTimerInt: {
			timer_interrupt();
			AfterInterrupt();
			return;
		}
		case causeInterrupt + sExternInt: {
			// TODO: get PLIC context ID mapping for HARD ID from FDT?
			uint64 irq = gPlicRegs->contexts[modeS + 2 * sBootHartId
				+ sPlicContextOfs].claimAndComplete;
			int_io_interrupt_handler(irq, true);
			gPlicRegs->contexts[modeS + 2*sBootHartId
				+ sPlicContextOfs].claimAndComplete = irq;
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
					WriteTrapInfo();
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
	WriteTrapInfo();
	panic("unhandled STrap");
}


//#pragma mark -

status_t
arch_int_init(kernel_args* args)
{
	sBootHartId = args->arch_args.bootHart;
	sPlicContextOfs = (sBootHartId == 0) ? 0 : -1;

	// TODO: Kernel mode FPU handling needs improved?
	SetStvec((uint64)SVec);
	SstatusReg sstatus(Sstatus());
	sstatus.ie = 0;
	sstatus.fs = extStatusInitial; // enable FPU
	sstatus.xs = extStatusOff;
	SetSstatus(sstatus.val);
	SetSie(Sie() | (1 << sTimerInt) | (1 << sExternInt));

	// TODO: read from FDT
	reserve_io_interrupt_vectors(128, 0, INTERRUPT_TYPE_IRQ);

	gPlicRegs->contexts[modeS + 2*sBootHartId + sPlicContextOfs].priorityThreshold = 0;
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
	gPlicRegs->enable[modeS + 2*sBootHartId + sPlicContextOfs][irq / 32] |= 1 << (irq % 32);
}


void
arch_int_disable_io_interrupt(int irq)
{
	dprintf("arch_int_disable_io_interrupt(%d)\n", irq);
	gPlicRegs->priority[irq] = 0;
	gPlicRegs->enable[modeS + 2*sBootHartId + sPlicContextOfs][irq / 32] &= ~(1 << (irq % 32));
}


void
arch_int_assign_to_cpu(int32 irq, int32 cpu)
{
	// SMP not yet supported
}
