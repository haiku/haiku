/*
 * Copyright 2008-2011, Michael Lotz, mmlr@mlotz.ch.
 * Copyright 2010, Clemens Zeidler, haiku@clemens-zeidler.de.
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <cpu.h>
#include <int.h>
#include <kscheduler.h>
#include <team.h>
#include <thread.h>
#include <util/AutoLock.h>

#include <arch/cpu.h>
#include <arch/int.h>

#include <arch/x86/apic.h>
#include <arch/x86/descriptors.h>
#include <arch/x86/msi.h>

#include <stdio.h>

// interrupt controllers
#include <arch/x86/ioapic.h>
#include <arch/x86/pic.h>


//#define TRACE_ARCH_INT
#ifdef TRACE_ARCH_INT
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


static const interrupt_controller* sCurrentPIC = NULL;


void
hardware_interrupt(struct iframe* frame)
{
#ifndef __x86_64__
	int32 vector = frame->vector - ARCH_INTERRUPT_BASE;
	bool levelTriggered = false;
	Thread* thread = thread_get_current_thread();

	if (sCurrentPIC->is_spurious_interrupt(vector)) {
		TRACE(("got spurious interrupt at vector %ld\n", vector));
		return;
	}

	levelTriggered = sCurrentPIC->is_level_triggered_interrupt(vector);

	if (!levelTriggered) {
		// if it's not handled by the current pic then it's an apic generated
		// interrupt like local interrupts, msi or ipi.
		if (!sCurrentPIC->end_of_interrupt(vector))
			apic_end_of_interrupt();
	}

	int_io_interrupt_handler(vector, levelTriggered);

	if (levelTriggered) {
		if (!sCurrentPIC->end_of_interrupt(vector))
			apic_end_of_interrupt();
	}

	cpu_status state = disable_interrupts();
	if (thread->cpu->invoke_scheduler) {
		SpinLocker schedulerLocker(gSchedulerLock);
		scheduler_reschedule();
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
#else
	panic("implement me");
#endif
}


// #pragma mark -


void
arch_int_enable_io_interrupt(int irq)
{
	sCurrentPIC->enable_io_interrupt(irq);
}


void
arch_int_disable_io_interrupt(int irq)
{
	sCurrentPIC->disable_io_interrupt(irq);
}


void
arch_int_configure_io_interrupt(int irq, uint32 config)
{
	sCurrentPIC->configure_io_interrupt(irq, config);
}


#undef arch_int_enable_interrupts
#undef arch_int_disable_interrupts
#undef arch_int_restore_interrupts
#undef arch_int_are_interrupts_enabled


void
arch_int_enable_interrupts(void)
{
	arch_int_enable_interrupts_inline();
}


int
arch_int_disable_interrupts(void)
{
	return arch_int_disable_interrupts_inline();
}


void
arch_int_restore_interrupts(int oldState)
{
	arch_int_restore_interrupts_inline(oldState);
}


bool
arch_int_are_interrupts_enabled(void)
{
	return arch_int_are_interrupts_enabled_inline();
}


status_t
arch_int_init_io(kernel_args* args)
{
#ifndef __x86_64__
	ioapic_init(args);
	msi_init();
#else
	panic("implement me");
#endif
	return B_OK;
}


status_t
arch_int_init_post_device_manager(kernel_args* args)
{
	return B_OK;
}


void
arch_int_set_interrupt_controller(const interrupt_controller& controller)
{
	sCurrentPIC = &controller;
}
