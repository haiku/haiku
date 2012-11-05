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


#define TRACE_ARCH_INT
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

// current fault handler
addr_t gFaultHandler;

// An iframe stack used in the early boot process when we don't have
// threads yet.
struct iframe_stack gBootFrameStack;


uint32
mmu_read_c1()
{
	uint32 controlReg = 0;
	asm volatile("MRC p15, 0, %[c1out], c1, c0, 0":[c1out] "=r" (controlReg));
	return controlReg;
}


void
mmu_write_c1(uint32 value)
{
	asm volatile("MCR p15, 0, %[c1in], c1, c0, 0"::[c1in] "r" (value));
}


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
print_iframe(struct iframe *frame)
{
}


status_t
arch_int_init(kernel_args *args)
{
	// see if high vectors are enabled
	if (mmu_read_c1() & (1<<13))
		dprintf("High vectors already enabled\n");
	else {
		mmu_write_c1(mmu_read_c1() | (1<<13));

		if (!(mmu_read_c1() & (1<<13)))
			dprintf("Unable to enable high vectors!\n");
		else
			dprintf("Enabled high vectors\n");
       }

	return B_OK;
}

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
	TRACE(("arch_int_init_io(%p)\n", args));
	return B_OK;
}


status_t
arch_int_init_post_device_manager(struct kernel_args *args)
{
	return B_ENTRY_NOT_FOUND;
}


extern "C" void arch_arm_undefined(struct iframe *iframe)
{
	panic("Undefined instruction!");
}

extern "C" void arch_arm_syscall(struct iframe *iframe)
{
	panic("Software interrupt!\n");
}

extern "C" void arch_arm_data_abort(struct iframe *iframe)
{
	addr_t newip;
	status_t res = vm_page_fault(iframe->r2 /* FAR */, iframe->r4 /* lr */,
		true /* TODO how to determine read/write? */,
		false /* only kernelspace for now */,
		&newip);

	if (res != B_HANDLED_INTERRUPT) {
		panic("Data Abort: %08x %08x %08x %08x (res=%lx)", iframe->r0 /* spsr */, 
			iframe->r1 /* FSR */, iframe->r2 /* FAR */,
			iframe->r4 /* lr */,
			res);
	} else {
		//panic("vm_page_fault was ok (%08lx/%08lx)!", iframe->r2 /* FAR */, iframe->r0 /* spsr */);
	}
}

extern "C" void arch_arm_prefetch_abort(struct iframe *iframe)
{
	panic("Prefetch Abort: %08x %08x %08x", iframe->r0, iframe->r1, iframe->r2);
}

extern "C" void arch_arm_irq(struct iframe *iframe)
{
	for (int i=0; i < 32; i++)
		if (sPxaInterruptBase[PXA_ICIP] & (1 << i))
			int_io_interrupt_handler(i, true);
}

extern "C" void arch_arm_fiq(struct iframe *iframe)
{
	for (int i=0; i < 32; i++)
		if (sPxaInterruptBase[PXA_ICIP] & (1 << i))
			dprintf("arch_arm_fiq: help me, FIQ %d was triggered but no FIQ support!\n", i);
}
