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
#include <vm/vm.h>
#include <vm/vm_priv.h>

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


static const char *kInterruptNames[] = {
	/*  0 */ "Divide Error Exception",
	/*  1 */ "Debug Exception",
	/*  2 */ "NMI Interrupt",
	/*  3 */ "Breakpoint Exception",
	/*  4 */ "Overflow Exception",
	/*  5 */ "BOUND Range Exceeded Exception",
	/*  6 */ "Invalid Opcode Exception",
	/*  7 */ "Device Not Available Exception",
	/*  8 */ "Double Fault Exception",
	/*  9 */ "Coprocessor Segment Overrun",
	/* 10 */ "Invalid TSS Exception",
	/* 11 */ "Segment Not Present",
	/* 12 */ "Stack Fault Exception",
	/* 13 */ "General Protection Exception",
	/* 14 */ "Page-Fault Exception",
	/* 15 */ "-",
	/* 16 */ "x87 FPU Floating-Point Error",
	/* 17 */ "Alignment Check Exception",
	/* 18 */ "Machine-Check Exception",
	/* 19 */ "SIMD Floating-Point Exception",
};
static const int kInterruptNameCount = 20;

static const interrupt_controller* sCurrentPIC = NULL;


static const char*
exception_name(int number, char* buffer, int32 bufferSize)
{
	if (number >= 0 && number < kInterruptNameCount)
		return kInterruptNames[number];

	snprintf(buffer, bufferSize, "exception %d", number);
	return buffer;
}


void
x86_invalid_exception(iframe* frame)
{
	Thread* thread = thread_get_current_thread();
	char name[32];
	panic("unhandled trap 0x%lx (%s) at ip 0x%lx, thread %" B_PRId32 "!\n",
		frame->vector, exception_name(frame->vector, name, sizeof(name)),
		frame->ip, thread ? thread->id : -1);
}


void
x86_fatal_exception(iframe* frame)
{
	char name[32];
	panic("Fatal exception \"%s\" occurred! Error code: 0x%lx\n",
		exception_name(frame->vector, name, sizeof(name)), frame->error_code);
}


void
x86_unexpected_exception(iframe* frame)
{
	debug_exception_type type;
	uint32 signalNumber;
	int32 signalCode;
	addr_t signalAddress = 0;
	int32 signalError = B_ERROR;

	switch (frame->vector) {
		case 0:		// Divide Error Exception (#DE)
			type = B_DIVIDE_ERROR;
			signalNumber = SIGFPE;
			signalCode = FPE_INTDIV;
			signalAddress = frame->ip;
			break;

		case 4:		// Overflow Exception (#OF)
			type = B_OVERFLOW_EXCEPTION;
			signalNumber = SIGFPE;
			signalCode = FPE_INTOVF;
			signalAddress = frame->ip;
			break;

		case 5:		// BOUND Range Exceeded Exception (#BR)
			type = B_BOUNDS_CHECK_EXCEPTION;
			signalNumber = SIGTRAP;
			signalCode = SI_USER;
			break;

		case 6:		// Invalid Opcode Exception (#UD)
			type = B_INVALID_OPCODE_EXCEPTION;
			signalNumber = SIGILL;
			signalCode = ILL_ILLOPC;
			signalAddress = frame->ip;
			break;

		case 13: 	// General Protection Exception (#GP)
			type = B_GENERAL_PROTECTION_FAULT;
			signalNumber = SIGILL;
			signalCode = ILL_PRVOPC;	// or ILL_PRVREG
			signalAddress = frame->ip;
			break;

		case 16: 	// x87 FPU Floating-Point Error (#MF)
			type = B_FLOATING_POINT_EXCEPTION;
			signalNumber = SIGFPE;
			signalCode = FPE_FLTDIV;
				// TODO: Determine the correct cause via the FPU status
				// register!
			signalAddress = frame->ip;
			break;

		case 17: 	// Alignment Check Exception (#AC)
			type = B_ALIGNMENT_EXCEPTION;
			signalNumber = SIGBUS;
			signalCode = BUS_ADRALN;
			// TODO: Also get the address (from where?). Since we don't enable
			// alignment checking this exception should never happen, though.
			signalError = EFAULT;
			break;

		case 19: 	// SIMD Floating-Point Exception (#XF)
			type = B_FLOATING_POINT_EXCEPTION;
			signalNumber = SIGFPE;
			signalCode = FPE_FLTDIV;
				// TODO: Determine the correct cause via the MXCSR register!
			signalAddress = frame->ip;
			break;

		default:
			x86_invalid_exception(frame);
			return;
	}

	if (IFRAME_IS_USER(frame)) {
		struct sigaction action;
		Thread* thread = thread_get_current_thread();

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
		char name[32];
		panic("Unexpected exception \"%s\" occurred in kernel mode! "
			"Error code: 0x%lx\n",
			exception_name(frame->vector, name, sizeof(name)),
			frame->error_code);
	}
}


void
x86_hardware_interrupt(struct iframe* frame)
{
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
}


void
x86_page_fault_exception(struct iframe* frame)
{
	Thread* thread = thread_get_current_thread();
	addr_t cr2 = x86_read_cr2();
	addr_t newip;

	if (debug_debugger_running()) {
		// If this CPU or this thread has a fault handler, we're allowed to be
		// here.
		if (thread != NULL) {
			cpu_ent* cpu = &gCPU[smp_get_current_cpu()];
			if (cpu->fault_handler != 0) {
				debug_set_page_fault_info(cr2, frame->ip,
					(frame->error_code & 0x2) != 0
						? DEBUG_PAGE_FAULT_WRITE : 0);
				frame->ip = cpu->fault_handler;
				frame->bp = cpu->fault_handler_stack_pointer;
				return;
			}

			if (thread->fault_handler != 0) {
				kprintf("ERROR: thread::fault_handler used in kernel "
					"debugger!\n");
				debug_set_page_fault_info(cr2, frame->ip,
					(frame->error_code & 0x2) != 0
						? DEBUG_PAGE_FAULT_WRITE : 0);
				frame->ip = thread->fault_handler;
				return;
			}
		}

		// otherwise, not really
		panic("page fault in debugger without fault handler! Touching "
			"address %p from ip %p\n", (void*)cr2, (void*)frame->ip);
		return;
	} else if ((frame->flags & 0x200) == 0) {
		// interrupts disabled

		// If a page fault handler is installed, we're allowed to be here.
		// TODO: Now we are generally allowing user_memcpy() with interrupts
		// disabled, which in most cases is a bug. We should add some thread
		// flag allowing to explicitly indicate that this handling is desired.
		if (thread && thread->fault_handler != 0) {
			if (frame->ip != thread->fault_handler) {
				frame->ip = thread->fault_handler;
				return;
			}

			// The fault happened at the fault handler address. This is a
			// certain infinite loop.
			panic("page fault, interrupts disabled, fault handler loop. "
				"Touching address %p from ip %p\n", (void*)cr2,
				(void*)frame->ip);
		}

		// If we are not running the kernel startup the page fault was not
		// allowed to happen and we must panic.
		panic("page fault, but interrupts were disabled. Touching address "
			"%p from ip %p\n", (void*)cr2, (void*)frame->ip);
		return;
	} else if (thread != NULL && thread->page_faults_allowed < 1) {
		panic("page fault not allowed at this place. Touching address "
			"%p from ip %p\n", (void*)cr2, (void*)frame->ip);
		return;
	}

	enable_interrupts();

	vm_page_fault(cr2, frame->ip,
		(frame->error_code & 0x2) != 0,	// write access
		(frame->error_code & 0x4) != 0,	// userland
		&newip);
	if (newip != 0) {
		// the page fault handler wants us to modify the iframe to set the
		// IP the cpu will return to this ip
		frame->ip = newip;
	}
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
	msi_init();
	ioapic_init(args);
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
