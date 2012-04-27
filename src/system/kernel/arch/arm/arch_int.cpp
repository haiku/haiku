/*
 * Copyright 2003-2010, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Axel Dörfler <axeld@pinc-software.de>
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 * 		François Revol <revol@free.fr>
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

/*typedef void (*m68k_exception_handler)(void);
#define M68K_EXCEPTION_VECTOR_COUNT 256
#warning M68K: align on 4 ?
//m68k_exception_handler gExceptionVectors[M68K_EXCEPTION_VECTOR_COUNT];
m68k_exception_handler *gExceptionVectors;

// defined in arch_exceptions.S
extern "C" void __m68k_exception_noop(void);
extern "C" void __m68k_exception_common(void);
*/
extern int __irqvec_start;
extern int __irqvec_end;

//extern"C" void m68k_exception_tail(void);

// current fault handler
addr_t gFaultHandler;

// An iframe stack used in the early boot process when we don't have
// threads yet.
struct iframe_stack gBootFrameStack;

// interrupt controller interface (initialized
// in arch_int_init_post_device_manager())
//static struct interrupt_controller_module_info *sPIC;
//static void *sPICCookie;


void
arch_int_enable_io_interrupt(int irq)
{
	#warning ARM WRITEME
	//if (!sPIC)
	//	return;

	// TODO: I have no idea, what IRQ type is appropriate.
	//sPIC->enable_io_interrupt(sPICCookie, irq, IRQ_TYPE_LEVEL);
//	M68KPlatform::Default()->EnableIOInterrupt(irq);
}


void
arch_int_disable_io_interrupt(int irq)
{
	#warning ARM WRITEME

	//if (!sPIC)
	//	return;

	//sPIC->disable_io_interrupt(sPICCookie, irq);
//	M68KPlatform::Default()->DisableIOInterrupt(irq);
}


/* arch_int_*_interrupts() and friends are in arch_asm.S */


static void
print_iframe(struct iframe *frame)
{
	#if 0
	dprintf("r0-r3:   0x%08lx 0x%08lx 0x%08lx 0x%08lx\n", frame->r0, frame->r1,
		frame->r2, frame->r3);
	dprintf("r4-r7:   0x%08lx 0x%08lx 0x%08lx 0x%08lx\n", frame->r4, frame->r5,
		frame->r6, frame->r7);
	dprintf("r8-r11:  0x%08lx 0x%08lx 0x%08lx 0x%08lx\n", frame->r8, frame->r9,
		frame->r10, frame->r11);
	dprintf("r12-r15: 0x%08lx 0x%08lx 0x%08lx 0x%08lx\n", frame->r12, frame->r13,
		frame->a6, frame->a7);
	dprintf("      pc 0x%08lx         sr 0x%08lx\n", frame->pc, frame->sr);
	#endif

	#warning ARM WRITEME
}


status_t
arch_int_init(kernel_args *args)
{
	#if 0
	status_t err;
	addr_t vbr;
	int i;

	gExceptionVectors = (m68k_exception_handler *)args->arch_args.vir_vbr;

	/* fill in the vector table */
	for (i = 0; i < M68K_EXCEPTION_VECTOR_COUNT; i++)
		gExceptionVectors[i] = &__m68k_exception_common;

	vbr = args->arch_args.phys_vbr;
	/* point VBR to the new table */
	asm volatile  ("movec %0,%%vbr" : : "r"(vbr):);
	#endif
	#warning ARM WRITEME

	return B_OK;
}


status_t
arch_int_init_post_vm(kernel_args *args)
{
	status_t err;
	// err = M68KPlatform::Default()->InitPIC(args);
	#warning ARM WRITEME

	return err;
}


status_t
arch_int_init_io(kernel_args* args)
{
	return B_OK;
}


status_t
arch_int_init_post_device_manager(struct kernel_args *args)
{
	// no PIC found
	panic("arch_int_init_post_device_manager(): Found no supported PIC!");

	return B_ENTRY_NOT_FOUND;
}
