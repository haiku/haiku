/*
 * Copyright 2003-2010, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 * 		François Revol <revol@free.fr>
 *		Jonas Sundström <jonas@kirilla.com>
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


// defined in arch_exceptions.S
extern int __irqvec_start;
extern int __irqvec_end;

// current fault handler
addr_t gFaultHandler;

// An iframe stack used in the early boot process when we don't have
// threads yet.
struct iframe_stack gBootFrameStack;


void
arch_int_enable_io_interrupt(int irq)
{
#warning IMPLEMENT arch_int_enable_io_interrupt
}


void
arch_int_disable_io_interrupt(int irq)
{
#warning IMPLEMENT arch_int_disable_io_interrupt
}


static void
print_iframe(struct iframe* frame)
{
#warning IMPLEMENT print_iframe
}


status_t
arch_int_init(kernel_args* args)
{
#warning IMPLEMENT arch_int_init
	return B_ERROR;
}


status_t
arch_int_init_post_vm(kernel_args* args)
{
#warning IMPLEMENT arch_int_init_post_vm
	return B_ERROR;
}


status_t
arch_int_init_io(kernel_args* args)
{
	return B_OK;
}


status_t
arch_int_init_post_device_manager(struct kernel_args* args)
{
#warning IMPLEMENT arch_int_init_post_device_manager
	panic("arch_int_init_post_device_manager()");
	return B_ENTRY_NOT_FOUND;
}


void
mipsel_get_current_cpu_exception_context(
	struct mipsel_cpu_exception_context *context)
{
#warning mipsel_get_current_cpu_exception_context
}


void
mipsel_set_current_cpu_exception_context(
	struct mipsel_cpu_exception_context *context)
{
#warning mipsel_set_current_cpu_exception_context
}

