/*
 * Copyright 2003-2012, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Axel Dörfler <axeld@pinc-software.de>
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 * 		François Revol <revol@free.fr>
 *              Ithamar R. Adema <ithamar@upgrade-android.com>
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <int.h>

#include <arch/smp.h>
#include <boot/kernel_args.h>
#include <device_manager.h>
#include <kscheduler.h>
#include <interrupt_controller.h>
#include <smp.h>
#include <thread.h>
#include <timer.h>
#include <util/DoublyLinkedList.h>
#include <util/kernel_cpp.h>
#include <vm/vm.h>
#include <vm/vm_priv.h>
#include <vm/VMAddressSpace.h>
#include <string.h>

#include <drivers/bus/FDT.h>
#include "soc.h"

#include "soc_pxa.h"
#include "soc_omap3.h"

#define TRACE_ARCH_INT
#ifdef TRACE_ARCH_INT
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define VECTORPAGE_SIZE		64
#define USER_VECTOR_ADDR_LOW	0x00000000
#define USER_VECTOR_ADDR_HIGH	0xffff0000

extern int _vectors_start;
extern int _vectors_end;

static area_id sVectorPageArea;
static void *sVectorPageAddress;
static area_id sUserVectorPageArea;
static void *sUserVectorPageAddress;
static fdt_module_info *sFdtModule;

// An iframe stack used in the early boot process when we don't have
// threads yet.
struct iframe_stack gBootFrameStack;


void
arch_int_enable_io_interrupt(int irq)
{
	TRACE(("arch_int_enable_io_interrupt(%d)\n", irq));
	InterruptController *ic = InterruptController::Get();
	if (ic != NULL)
		ic->EnableInterrupt(irq);
}


void
arch_int_disable_io_interrupt(int irq)
{
	TRACE(("arch_int_disable_io_interrupt(%d)\n", irq));
	InterruptController *ic = InterruptController::Get();
	if (ic != NULL)
		ic->DisableInterrupt(irq);
}


/* arch_int_*_interrupts() and friends are in arch_asm.S */

void
arch_int_assign_to_cpu(int32 irq, int32 cpu)
{
	// intentionally left blank; no SMP support (yet)
}

static void
print_iframe(const char *event, struct iframe *frame)
{
	if (event)
		dprintf("Exception: %s\n", event);

	dprintf("R00=%08lx R01=%08lx R02=%08lx R03=%08lx\n"
		"R04=%08lx R05=%08lx R06=%08lx R07=%08lx\n",
		frame->r0, frame->r1, frame->r2, frame->r3,
		frame->r4, frame->r5, frame->r6, frame->r7);
	dprintf("R08=%08lx R09=%08lx R10=%08lx R11=%08lx\n"
		"R12=%08lx SP=%08lx LR=%08lx  PC=%08lx CPSR=%08lx\n",
		frame->r8, frame->r9, frame->r10, frame->r11,
		frame->r12, frame->svc_sp, frame->svc_lr, frame->pc, frame->spsr);
}


status_t
arch_int_init(kernel_args *args)
{
	return B_OK;
}


extern "C" void arm_vector_init(void);


static struct fdt_device_info intc_table[] = {
	{
		.compatible = "marvell,pxa-intc",
		.init = PXAInterruptController::Init,
	}, {
		.compatible = "ti,omap3-intc",
		.init = OMAP3InterruptController::Init,
	}
};
static int intc_count = sizeof(intc_table) / sizeof(struct fdt_device_info);


status_t
arch_int_init_post_vm(kernel_args *args)
{
	// create a read/write kernel area
	sVectorPageArea = create_area("vectorpage", (void **)&sVectorPageAddress,
		B_ANY_ADDRESS, VECTORPAGE_SIZE, B_FULL_LOCK,
		B_KERNEL_WRITE_AREA | B_KERNEL_READ_AREA);
	if (sVectorPageArea < 0)
		panic("vector page could not be created!");

	// clone it at a fixed address with user read/only permissions
	sUserVectorPageAddress = (addr_t*)USER_VECTOR_ADDR_HIGH;
	sUserVectorPageArea = clone_area("user_vectorpage",
		(void **)&sUserVectorPageAddress, B_EXACT_ADDRESS,
		B_READ_AREA | B_EXECUTE_AREA, sVectorPageArea);

	if (sUserVectorPageArea < 0)
		panic("user vector page @ %p could not be created (%lx)!",
			sVectorPageAddress, sUserVectorPageArea);

	// copy vectors into the newly created area
	memcpy(sVectorPageAddress, &_vectors_start, VECTORPAGE_SIZE);

	arm_vector_init();

	// see if high vectors are enabled
	if ((mmu_read_c1() & (1 << 13)) != 0)
		dprintf("High vectors already enabled\n");
	else {
		mmu_write_c1(mmu_read_c1() | (1 << 13));

		if ((mmu_read_c1() & (1 << 13)) == 0)
			dprintf("Unable to enable high vectors!\n");
		else
			dprintf("Enabled high vectors\n");
	}

	status_t rc = get_module(B_FDT_MODULE_NAME, (module_info**)&sFdtModule);
	if (rc != B_OK)
		panic("Unable to get FDT module: %08lx!\n", rc);

	rc = sFdtModule->setup_devices(intc_table, intc_count, NULL);
	if (rc != B_OK)
		panic("No interrupt controllers found!\n");

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


// Little helper class for handling the
// iframe stack as used by KDL.
class IFrameScope {
public:
	IFrameScope(struct iframe *iframe) {
		fThread = thread_get_current_thread();
		if (fThread)
			arm_push_iframe(&fThread->arch_info.iframes, iframe);
		else
			arm_push_iframe(&gBootFrameStack, iframe);
	}

	virtual ~IFrameScope() {
		// pop iframe
		if (fThread)
			arm_pop_iframe(&fThread->arch_info.iframes);
		else
			arm_pop_iframe(&gBootFrameStack);
	}
private:
	Thread* fThread;
};


extern "C" void
arch_arm_undefined(struct iframe *iframe)
{
	print_iframe("Undefined Instruction", iframe);
	IFrameScope scope(iframe); // push/pop iframe

	panic("not handled!");
}


extern "C" void
arch_arm_syscall(struct iframe *iframe)
{
	print_iframe("Software interrupt", iframe);
	IFrameScope scope(iframe); // push/pop iframe
}


extern "C" void
arch_arm_data_abort(struct iframe *frame)
{
	Thread *thread = thread_get_current_thread();
	bool isUser = (frame->spsr & 0x1f) == 0x10;
	addr_t far = arm_get_far();
	bool isWrite = true;
	addr_t newip = 0;

#ifdef TRACE_ARCH_INT
	print_iframe("Data Abort", frame);
	dprintf("FAR: %08lx, thread: %s\n", far, thread->name);
#endif

	IFrameScope scope(frame);

	if (debug_debugger_running()) {
		// If this CPU or this thread has a fault handler, we're allowed to be
		// here.
		if (thread != NULL) {
			cpu_ent* cpu = &gCPU[smp_get_current_cpu()];

			if (cpu->fault_handler != 0) {
				debug_set_page_fault_info(far, frame->pc,
					isWrite ? DEBUG_PAGE_FAULT_WRITE : 0);
				frame->svc_sp = cpu->fault_handler_stack_pointer;
				frame->pc = cpu->fault_handler;
				return;
			}

			if (thread->fault_handler != 0) {
				kprintf("ERROR: thread::fault_handler used in kernel "
					"debugger!\n");
				debug_set_page_fault_info(far, frame->pc,
						isWrite ? DEBUG_PAGE_FAULT_WRITE : 0);
				frame->pc = reinterpret_cast<uintptr_t>(thread->fault_handler);
				return;
			}
		}

		// otherwise, not really
		panic("page fault in debugger without fault handler! Touching "
			"address %p from pc %p\n", (void *)far, (void *)frame->pc);
		return;
	} else if ((frame->spsr & (1 << 7)) != 0) {
		// interrupts disabled

		// If a page fault handler is installed, we're allowed to be here.
		// TODO: Now we are generally allowing user_memcpy() with interrupts
		// disabled, which in most cases is a bug. We should add some thread
		// flag allowing to explicitly indicate that this handling is desired.
		uintptr_t handler = reinterpret_cast<uintptr_t>(thread->fault_handler);
		if (thread && thread->fault_handler != 0) {
			if (frame->pc != handler) {
				frame->pc = handler;
				return;
			}

			// The fault happened at the fault handler address. This is a
			// certain infinite loop.
			panic("page fault, interrupts disabled, fault handler loop. "
				"Touching address %p from pc %p\n", (void*)far,
				(void*)frame->pc);
		}

		// If we are not running the kernel startup the page fault was not
		// allowed to happen and we must panic.
		panic("page fault, but interrupts were disabled. Touching address "
			"%p from pc %p\n", (void *)far, (void *)frame->pc);
		return;
	} else if (thread != NULL && thread->page_faults_allowed < 1) {
		panic("page fault not allowed at this place. Touching address "
			"%p from pc %p\n", (void *)far, (void *)frame->pc);
		return;
	}

	enable_interrupts();

	vm_page_fault(far, frame->pc, isWrite, false, isUser, &newip);

	if (newip != 0) {
		// the page fault handler wants us to modify the iframe to set the
		// IP the cpu will return to to be this ip
		frame->pc = newip;
	}
}


extern "C" void
arch_arm_prefetch_abort(struct iframe *iframe)
{
	print_iframe("Prefetch Abort", iframe);
	IFrameScope scope(iframe);

	panic("not handled!");
}


extern "C" void
arch_arm_irq(struct iframe *iframe)
{
	IFrameScope scope(iframe);

	InterruptController *ic = InterruptController::Get();
	if (ic != NULL)
		ic->HandleInterrupt();
}


extern "C" void
arch_arm_fiq(struct iframe *iframe)
{
	IFrameScope scope(iframe);

	panic("FIQ not implemented yet!");
}
