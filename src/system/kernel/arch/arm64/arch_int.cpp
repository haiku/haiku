/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
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


void
arch_int_enable_io_interrupt(int irq)
{
}


void
arch_int_disable_io_interrupt(int irq)
{
}


void
arch_int_assign_to_cpu(int32 irq, int32 cpu)
{
	// intentionally left blank; no SMP support (yet)
}


status_t
arch_int_init(kernel_args *args)
{
	return B_OK;
}


status_t
arch_int_init_post_vm(kernel_args *args)
{
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
