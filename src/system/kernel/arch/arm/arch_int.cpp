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


//#define TRACE_ARCH_INT
#ifdef TRACE_ARCH_INT
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define VECTORPAGE_SIZE		64
#define USER_VECTOR_ADDR_LOW	0x00000000
#define USER_VECTOR_ADDR_HIGH	0xffff0000

#define PXA_INTERRUPT_PHYS_BASE	0x40D00000
#define PXA_INTERRUPT_SIZE	0x00000034

#define PXA_ICIP	0x00
#define PXA_ICMR	0x01
#define PXA_ICFP	0x03
#define PXA_ICMR2	0x28

static area_id sPxaInterruptArea;
static uint32 *sPxaInterruptBase;

extern int _vectors_start;
extern int _vectors_end;

static area_id sVectorPageArea;
static void *sVectorPageAddress;
static area_id sUserVectorPageArea;
static void *sUserVectorPageAddress;

// An iframe stack used in the early boot process when we don't have
// threads yet.
struct iframe_stack gBootFrameStack;


void
arch_int_enable_io_interrupt(int irq)
{
	TRACE(("arch_int_enable_io_interrupt(%d)\n", irq));

	if (irq <= 31) {
		sPxaInterruptBase[PXA_ICMR] |= 1 << irq;
		return;
	}

	sPxaInterruptBase[PXA_ICMR2] |= 1 << (irq - 32);
}


void
arch_int_disable_io_interrupt(int irq)
{
	TRACE(("arch_int_disable_io_interrupt(%d)\n", irq));

	if (irq <= 31) {
		sPxaInterruptBase[PXA_ICMR] &= ~(1 << irq);
		return;
	}

	sPxaInterruptBase[PXA_ICMR2] &= ~(1 << (irq - 32));
}


/* arch_int_*_interrupts() and friends are in arch_asm.S */


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
		"R12=%08lx R13=%08lx R14=%08lx CPSR=%08lx\n",
		frame->r8, frame->r9, frame->r10, frame->r11,
		frame->r12, frame->usr_sp, frame->usr_lr, frame->spsr);
}


status_t
arch_int_init(kernel_args *args)
{
	return B_OK;
}


extern "C" void arm_vector_init(void);


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
		panic("user vector page @ %p could not be created (%lx)!", sVectorPageAddress, sUserVectorPageArea);

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

	sPxaInterruptArea = map_physical_memory("pxa_intc", PXA_INTERRUPT_PHYS_BASE,
		PXA_INTERRUPT_SIZE, 0, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, (void**)&sPxaInterruptBase);

	if (sPxaInterruptArea < 0)
		return sPxaInterruptArea;

	sPxaInterruptBase[PXA_ICMR] = 0;
	sPxaInterruptBase[PXA_ICMR2] = 0;

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


extern "C" void
arch_arm_undefined(struct iframe *iframe)
{
	print_iframe("Undefined Instruction", iframe);
	panic("not handled!");
}


extern "C" void
arch_arm_syscall(struct iframe *iframe)
{
	print_iframe("Software interrupt", iframe);
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
#endif

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
				frame->pc = thread->fault_handler;
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
		if (thread && thread->fault_handler != 0) {
			if (frame->pc != thread->fault_handler) {
				frame->pc = thread->fault_handler;
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

	vm_page_fault(far, frame->pc, isWrite, isUser, &newip);

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
	panic("not handled!");
}


extern "C" void
arch_arm_irq(struct iframe *iframe)
{
	for (int i=0; i < 32; i++) {
		if (sPxaInterruptBase[PXA_ICIP] & (1 << i))
			int_io_interrupt_handler(i, true);
	}
}


extern "C" void
arch_arm_fiq(struct iframe *iframe)
{
	for (int i=0; i < 32; i++) {
		if (sPxaInterruptBase[PXA_ICIP] & (1 << i)) {
			dprintf("arch_arm_fiq: help me, FIQ %d was triggered but no "
				"FIQ support!\n", i);
		}
	}
}
