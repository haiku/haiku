/*
 * Copyright 2019-2022 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#include <int.h>

#include <arch/smp.h>
#include <boot/kernel_args.h>
#include <device_manager.h>
#include <kscheduler.h>
#include <ksyscalls.h>
#include <interrupt_controller.h>
#include <smp.h>
#include <thread.h>
#include <timer.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <util/kernel_cpp.h>
#include <vm/vm.h>
#include <vm/vm_priv.h>
#include <vm/VMAddressSpace.h>
#include "syscall_numbers.h"
#include "VMSAv8TranslationMap.h"
#include <string.h>

#include "soc.h"
#include "arch_int_gicv2.h"

#define TRACE_ARCH_INT
#ifdef TRACE_ARCH_INT
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


void
arch_int_enable_io_interrupt(int irq)
{
	InterruptController *ic = InterruptController::Get();
	if (ic != NULL)
		ic->EnableInterrupt(irq);
}


void
arch_int_disable_io_interrupt(int irq)
{
	InterruptController *ic = InterruptController::Get();
	if (ic != NULL)
		ic->DisableInterrupt(irq);
}


int32
arch_int_assign_to_cpu(int32 irq, int32 cpu)
{
	// Not yet supported.
	return 0;
}


status_t
arch_int_init(kernel_args *args)
{
	reserve_io_interrupt_vectors(128, 32, INTERRUPT_TYPE_IRQ);
	return B_OK;
}


status_t
arch_int_init_post_vm(kernel_args *args)
{
	InterruptController *ic = NULL;
	if (strcmp(args->arch_args.interrupt_controller.kind, INTC_KIND_GICV2) == 0) {
		ic = new(std::nothrow) GICv2InterruptController(
			args->arch_args.interrupt_controller.regs1.start,
			args->arch_args.interrupt_controller.regs2.start);
	}

	if (ic == NULL)
		return B_ERROR;

	return B_OK;
}


status_t
arch_int_init_io(kernel_args* args)
{
	return B_OK;
}


status_t
arch_int_init_post_device_manager(struct kernel_args *args)
{
	return B_ENTRY_NOT_FOUND;
}


// TODO: reuse things from VMSAv8TranslationMap


static int page_bits = 12;

static constexpr uint64_t kPteAddrMask = (((1UL << 36) - 1) << 12);
static constexpr uint64_t kPteAttrMask = ~(kPteAddrMask | 0x3);
static constexpr uint64_t kAttrSWDBM = (1UL << 55);
static constexpr uint64_t kAttrAF = (1UL << 10);
static constexpr uint64_t kAttrAP2 = (1UL << 7);


static uint64_t*
TableFromPa(phys_addr_t pa)
{
	return reinterpret_cast<uint64_t*>(KERNEL_PMAP_BASE + pa);
}


static bool
fixup_entry(phys_addr_t ptPa, int level, addr_t va, bool wr)
{
	int tableBits = page_bits - 3;
	uint64_t tableMask = (1UL << tableBits) - 1;

	int shift = tableBits * (3 - level) + page_bits;
	uint64_t entrySize = 1UL << shift;
	uint64_t entryMask = entrySize - 1;

	int index = (va >> shift) & tableMask;

	uint64_t *pte = &TableFromPa(ptPa)[index];

	int type = *pte & 0x3;
	uint64_t addr = *pte & kPteAddrMask;

	if ((level == 3 && type == 0x3) || (level < 3 && type == 0x1)) {
		if (!wr && (*pte & kAttrAF) == 0) {
			atomic_or64((int64*)pte, kAttrAF);
			return true;
		}
		if (wr && (*pte & kAttrSWDBM) != 0 && (*pte & kAttrAP2) != 0) {
			atomic_and64((int64*)pte, ~kAttrAP2);
			asm("tlbi vaae1is, %0 \n dsb ish"::"r"(va >> page_bits));
			return true;
		}
	} else if (level < 3 && type == 0x3) {
		return fixup_entry(addr, level + 1, va, wr);
	}

	return false;
}


void
after_exception()
{
	Thread* thread = thread_get_current_thread();
	if (thread->cpu->invoke_scheduler) {
		disable_interrupts();
		SpinLocker schedulerLocker(thread->scheduler_lock);
		scheduler_reschedule(B_THREAD_READY);
	}
}


extern "C" void
do_sync_handler(iframe * frame)
{
	bool isExec = false;
	switch (ESR_ELx_EXCEPTION(frame->esr)) {
		case EXCP_INSN_ABORT_L:
		case EXCP_INSN_ABORT:
			isExec = true;
		case EXCP_DATA_ABORT_L:
		case EXCP_DATA_ABORT:
		{
			bool write = (frame->esr & ISS_DATA_WnR) != 0;
			bool known = false;

			int initialLevel = VMSAv8TranslationMap::CalcStartLevel(48, 12);
			phys_addr_t ptPa;
			bool addrType = (frame->far & (1UL << 63)) != 0;
			if (addrType)
				ptPa = READ_SPECIALREG(TTBR1_EL1);
			else
				ptPa = READ_SPECIALREG(TTBR0_EL1);

			switch (frame->esr & ISS_DATA_DFSC_MASK) {
				case ISS_DATA_DFSC_TF_L0:
				case ISS_DATA_DFSC_TF_L1:
				case ISS_DATA_DFSC_TF_L2:
				case ISS_DATA_DFSC_TF_L3:
					known = true;
				break;

				case ISS_DATA_DFSC_AFF_L1:
				case ISS_DATA_DFSC_AFF_L2:
				case ISS_DATA_DFSC_AFF_L3:
					known = true;
					if (fixup_entry(ptPa, initialLevel, frame->far, false))
						return;
				break;

				case ISS_DATA_DFSC_PF_L1:
				case ISS_DATA_DFSC_PF_L2:
				case ISS_DATA_DFSC_PF_L3:
					known = true;
					if (write && fixup_entry(ptPa, initialLevel, frame->far, true))
						return;
				break;
			}

			if (!known)
				break;

			if (debug_debugger_running()) {
				Thread* thread = thread_get_current_thread();
				if (thread != NULL) {
					cpu_ent* cpu = &gCPU[smp_get_current_cpu()];
					if (cpu->fault_handler != 0) {
						debug_set_page_fault_info(frame->far, frame->elr,
							write ? DEBUG_PAGE_FAULT_WRITE : 0);
						frame->elr = cpu->fault_handler;
						frame->sp = cpu->fault_handler_stack_pointer;
						return;
					}
				}
			}

			Thread *thread = thread_get_current_thread();
			ASSERT(thread);

			bool isUser = (frame->spsr & PSR_M_MASK) == PSR_M_EL0t;

			if ((frame->spsr & PSR_I) != 0) {
				// interrupts disabled
				uintptr_t handler = reinterpret_cast<uintptr_t>(thread->fault_handler);
				if (thread->fault_handler != 0) {
					frame->elr = handler;
					return;
				}
			} else if (thread->page_faults_allowed != 0) {
				dprintf("PF: %lx\n", frame->far);
				enable_interrupts();
				addr_t ret = 0;
				vm_page_fault(frame->far, frame->elr, write, isExec, isUser, &ret);
				if (ret != 0)
					frame->elr = ret;
				return;
			}

			panic("unhandled pagefault! FAR=%lx ELR=%lx ESR=%lx",
				frame->far, frame->elr, frame->esr);
			break;
		}

		case EXCP_SVC64:
		{
			uint32 imm = (frame->esr & 0xffff);

			uint32 count = imm & 0x1f;
			uint32 syscall = imm >> 5;

			uint64_t args[20];
			if (count > 20) {
				frame->x[0] = B_ERROR;
				return;
			}

			memset(args, 0, sizeof(args));
			memcpy(args, frame->x, (count < 8 ? count : 8) * 8);

			if (count > 8) {
				if (!IS_USER_ADDRESS(frame->sp)
					|| user_memcpy(&args[8], (void*)frame->sp, (count - 8) * 8) != B_OK) {
					frame->x[0] = B_BAD_ADDRESS;
					return;
				}
			}

			thread_at_kernel_entry(system_time());

			enable_interrupts();
			syscall_dispatcher(syscall, (void*)args, &frame->x[0]);

			{
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
					panic("syscall restart");
				}
			}

			return;
		}
	}

	panic("unhandled exception! FAR=%lx ELR=%lx ESR=%lx (EC=%lx)",
		frame->far, frame->elr, frame->esr, (frame->esr >> 26) & 0x3f);
}


extern "C" void
do_error_handler(iframe * frame)
{
	panic("unhandled error! FAR=%lx ELR=%lx ESR=%lx", frame->far, frame->elr, frame->esr);
}


extern "C" void
do_irq_handler(iframe * frame)
{
	InterruptController *ic = InterruptController::Get();
	if (ic != NULL)
		ic->HandleInterrupt();

	after_exception();
}


extern "C" void
do_fiq_handler(iframe * frame)
{
	panic("do_fiq_handler");
}
