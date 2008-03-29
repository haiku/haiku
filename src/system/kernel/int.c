/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <int.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arch/debug_console.h>
#include <arch/int.h>
#include <boot/kernel_args.h>
#include <util/kqueue.h>
#include <smp.h>


//#define TRACE_INT
#ifdef TRACE_INT
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define DEBUG_INT
	// adds statistics and unhandled counter

struct io_handler {
	struct io_handler	*next;
	struct io_handler	*prev;
	interrupt_handler	func;
	void				*data;
	bool				use_enable_counter;
};

struct io_vector {
	struct io_handler	handler_list;
	spinlock			vector_lock;
	int32				enable_count;
	bool				no_lock_vector;
#ifdef DEBUG_INT
	int64				handled_count;
	int64				unhandled_count;
	int					trigger_count;
	int					ignored_count;
#endif
};

static struct io_vector sVectors[NUM_IO_VECTORS];


#ifdef DEBUG_INT
static int
dump_int_statistics(int argc, char **argv)
{
	int i;
	for (i = 0; i < NUM_IO_VECTORS; i++) {
		struct io_handler *io;

		if (sVectors[i].vector_lock == 0
			&& sVectors[i].enable_count == 0
			&& sVectors[i].handled_count == 0
			&& sVectors[i].unhandled_count == 0
			&& sVectors[i].handler_list.next == &sVectors[i].handler_list)
			continue;

		kprintf("int %3d, enabled %ld, handled %8lld, unhandled %8lld%s%s\n",
			i, sVectors[i].enable_count, sVectors[i].handled_count, 
			sVectors[i].unhandled_count,
			sVectors[i].vector_lock != 0 ? ", ACTIVE" : "",
			sVectors[i].handler_list.next == &sVectors[i].handler_list
				? ", no handler" : "");

		for (io = sVectors[i].handler_list.next;
				io != &sVectors[i].handler_list; io = io->next) {
			kprintf("\t%p", io->func);
		}
		if (sVectors[i].handler_list.next != &sVectors[i].handler_list)
			kprintf("\n");
	}
	return 0;
}
#endif


//	#pragma mark - private kernel API


bool
interrupts_enabled(void)
{
	return arch_int_are_interrupts_enabled();
}


status_t
int_init(kernel_args *args)
{
	TRACE(("init_int_handlers: entry\n"));

	return arch_int_init(args);
}


status_t
int_init_post_vm(kernel_args *args)
{
	int i;

	/* initialize the vector list */
	for (i = 0; i < NUM_IO_VECTORS; i++) {
		sVectors[i].vector_lock = 0;			/* initialize spinlock */
		sVectors[i].enable_count = 0;
		sVectors[i].no_lock_vector = false;
#ifdef DEBUG_INT
		sVectors[i].handled_count = 0;
		sVectors[i].unhandled_count = 0;
		sVectors[i].trigger_count = 0;
		sVectors[i].ignored_count = 0;
#endif
		initque(&sVectors[i].handler_list);	/* initialize handler queue */
	}

#ifdef DEBUG_INT
	add_debugger_command("ints", &dump_int_statistics,
		"list interrupt statistics");
#endif

	return arch_int_init_post_vm(args);
}


status_t
int_init_post_device_manager(kernel_args *args)
{
	arch_debug_install_interrupt_handlers();

	return arch_int_init_post_device_manager(args);
}


/*!	Actually process an interrupt via the handlers registered for that
	vector (IRQ).
*/
int
int_io_interrupt_handler(int vector, bool levelTriggered)
{
	int status = B_UNHANDLED_INTERRUPT;
	struct io_handler *io;
	bool invokeScheduler = false, handled = false;

	if (!sVectors[vector].no_lock_vector)
		acquire_spinlock(&sVectors[vector].vector_lock);

	// The list can be empty at this place
	if (sVectors[vector].handler_list.next == &sVectors[vector].handler_list) {
		dprintf("unhandled io interrupt %d\n", vector);
		if (!sVectors[vector].no_lock_vector)
			release_spinlock(&sVectors[vector].vector_lock);
		return B_UNHANDLED_INTERRUPT;
	}

	/* For level-triggered interrupts, we actually handle the return
	 * value (ie. B_HANDLED_INTERRUPT) to decide wether or not we
	 * want to call another interrupt handler.
	 * For edge-triggered interrupts, however, we always need to call
	 * all handlers, as multiple interrupts cannot be identified. We
	 * still make sure the return code of this function will issue
	 * whatever the driver thought would be useful (ie. B_INVOKE_SCHEDULER)
	 */

	for (io = sVectors[vector].handler_list.next;
			io != &sVectors[vector].handler_list;
			io = io->next) {
		status = io->func(io->data);

		if (levelTriggered && status != B_UNHANDLED_INTERRUPT)
			break;

		if (status == B_HANDLED_INTERRUPT)
			handled = true;
		else if (status == B_INVOKE_SCHEDULER)
			invokeScheduler = true;
	}

#ifdef DEBUG_INT
	sVectors[vector].trigger_count++;
	if (status != B_UNHANDLED_INTERRUPT || handled || invokeScheduler) {
		sVectors[vector].handled_count++;
	} else {
		sVectors[vector].unhandled_count++;
		sVectors[vector].ignored_count++;
	}

	if (sVectors[vector].trigger_count > 10000) {
		if (sVectors[vector].ignored_count > 9900) {
			if (sVectors[vector].handler_list.next == NULL
				|| sVectors[vector].handler_list.next->next == NULL) {
				// this interrupt vector is not shared, disable it
				sVectors[vector].enable_count = -100;
				arch_int_disable_io_interrupt(vector);
				dprintf("Disabling unhandled io interrupt %d\n", vector);
			} else {
				// this is a shared interrupt vector, we cannot just disable it
				dprintf("More than 99%% interrupts of vector %d are unhandled\n",
					vector);
			}
		}

		sVectors[vector].trigger_count = 0;
		sVectors[vector].ignored_count = 0;
	}
#endif

	if (!sVectors[vector].no_lock_vector)
		release_spinlock(&sVectors[vector].vector_lock);

	if (levelTriggered)
		return status;

	// edge triggered return value

	if (invokeScheduler)
		return B_INVOKE_SCHEDULER;
	if (handled)
		return B_HANDLED_INTERRUPT;

	return B_UNHANDLED_INTERRUPT;
}


//	#pragma mark - public API


cpu_status
disable_interrupts(void)
{
	return arch_int_disable_interrupts();
}


void
restore_interrupts(cpu_status status)
{
	arch_int_restore_interrupts(status);
}


/*!	Install a handler to be called when an interrupt is triggered
	for the given interrupt number with \a data as the argument.
*/
status_t
install_io_interrupt_handler(long vector, interrupt_handler handler, void *data,
	ulong flags)
{
	struct io_handler *io = NULL; 
	cpu_status state;

	if (vector < 0 || vector >= NUM_IO_VECTORS)
		return B_BAD_VALUE;

	io = (struct io_handler *)malloc(sizeof(struct io_handler));
	if (io == NULL)
		return B_NO_MEMORY;

	arch_debug_remove_interrupt_handler(vector);
		// There might be a temporary debug interrupt installed on this
		// vector that should be removed now.

	io->func = handler;
	io->data = data;
	io->use_enable_counter = (flags & B_NO_ENABLE_COUNTER) == 0;

	// Disable the interrupts, get the spinlock for this irq only
	// and then insert the handler
	state = disable_interrupts();
	acquire_spinlock(&sVectors[vector].vector_lock);

	insque(io, &sVectors[vector].handler_list);

	// If B_NO_ENABLE_COUNTER is set, we're being asked to not alter
	// whether the interrupt should be enabled or not
	if (io->use_enable_counter) {
		if (sVectors[vector].enable_count++ == 0)
			arch_int_enable_io_interrupt(vector);
	}

	// If B_NO_LOCK_VECTOR is specified this is a vector that is not supposed
	// to have multiple handlers and does not require locking of the vector
	// when entering the handler. For example this is used by internally
	// registered interrupt handlers like for handling local APIC interrupts
	// that may run concurently on multiple CPUs. Locking with a spinlock
	// would in that case defeat the purpose as it would serialize calling the
	// handlers in parallel on different CPUs.
	if (flags & B_NO_LOCK_VECTOR)
		sVectors[vector].no_lock_vector = true;

	release_spinlock(&sVectors[vector].vector_lock);
	restore_interrupts(state);

	return B_OK;
}


/*!	Remove a previously installed interrupt handler */
status_t
remove_io_interrupt_handler(long vector, interrupt_handler handler, void *data)
{
	status_t status = B_BAD_VALUE;
	struct io_handler *io = NULL; 
	cpu_status state;

	if (vector < 0 || vector >= NUM_IO_VECTORS)
		return B_BAD_VALUE;

	/* lock the structures down so it is not modified while we search */
	state = disable_interrupts();
	acquire_spinlock(&sVectors[vector].vector_lock);

	/* loop through the available handlers and try to find a match.
	 * We go forward through the list but this means we start with the
	 * most recently added handlers.
	 */
	for (io = sVectors[vector].handler_list.next;
	     io != &sVectors[vector].handler_list;
	     io = io->next) {
		/* we have to match both function and data */
		if (io->func == handler && io->data == data) {
			remque(io);

			// Check if we need to disable the interrupt
			if (io->use_enable_counter && --sVectors[vector].enable_count == 0)
				arch_int_disable_io_interrupt(vector);

			status = B_OK;
			break;
		}
	}

	release_spinlock(&sVectors[vector].vector_lock);
	restore_interrupts(state);

	// if the handler could be found and removed, we still have to free it
	if (status == B_OK)
		free(io);

	return status;
} 

