/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <int.h>
#include <smp.h>
#include <util/kqueue.h>
#include <boot/kernel_args.h>
#include <arch/int.h>

#include <string.h>
#include <stdio.h>
#include <malloc.h>

//#define TRACE_INT
#ifdef TRACE_INT
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define DEBUG_INT

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
#ifdef DEBUG_INT
	int64				handled_count;
	int64				unhandled_count;
#endif
};

static struct io_vector io_vectors[NUM_IO_VECTORS];


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


bool
interrupts_enabled(void)
{
	return arch_int_are_interrupts_enabled();
}


#ifdef DEBUG_INT
static int
dump_int_statistics(int argc, char **argv)
{
	int i;
	for (i = 0; i < NUM_IO_VECTORS; i++) {
		if (io_vectors[i].vector_lock == 0
			&& io_vectors[i].enable_count == 0
			&& io_vectors[i].handled_count == 0
			&& io_vectors[i].unhandled_count == 0
			&& io_vectors[i].handler_list.next == &io_vectors[i].handler_list)
			continue;
		kprintf("int %3d, enabled %ld, handled %8lld, unhandled %8lld%s%s\n",
			i, 
			io_vectors[i].enable_count, 
			io_vectors[i].handled_count, 
			io_vectors[i].unhandled_count,
			(io_vectors[i].vector_lock != 0) ? ", ACTIVE" : "",
			(io_vectors[i].handler_list.next == &io_vectors[i].handler_list) ? ", no handler" : "");
	}
	return 0;
}
#endif


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
		io_vectors[i].vector_lock = 0;			/* initialize spinlock */
		io_vectors[i].enable_count = 0;
		#ifdef DEBUG_INT
			io_vectors[i].handled_count = 0;
			io_vectors[i].unhandled_count = 0;
		#endif
		initque(&io_vectors[i].handler_list);	/* initialize handler queue */
	}

#ifdef DEBUG_INT
	add_debugger_command("ints", &dump_int_statistics, "list interrupt statistics");
#endif

	return arch_int_init_post_vm(args);
}


/** Install a handler to be called when an interrupt is triggered
 *	for the given interrupt number with \a data as the argument.
 */

status_t
install_io_interrupt_handler(long vector, interrupt_handler handler, void *data, ulong flags)
{
	struct io_handler *io = NULL; 
	cpu_status state;

	if (vector < 0 || vector >= NUM_IO_VECTORS)
		return B_BAD_VALUE;

	io = (struct io_handler *)malloc(sizeof(struct io_handler));
	if (io == NULL)
		return B_NO_MEMORY;

	io->func = handler;
	io->data = data;
	io->use_enable_counter = (flags & B_NO_ENABLE_COUNTER) == 0;

	// Disable the interrupts, get the spinlock for this irq only
	// and then insert the handler
	state = disable_interrupts();
	acquire_spinlock(&io_vectors[vector].vector_lock);

	insque(io, &io_vectors[vector].handler_list);

	// If B_NO_ENABLE_COUNTER is set, we're being asked to not alter
	// whether the interrupt should be enabled or not
	if (io->use_enable_counter) {
		if (io_vectors[vector].enable_count++ == 0)
			arch_int_enable_io_interrupt(vector);
	}

	release_spinlock(&io_vectors[vector].vector_lock);
	restore_interrupts(state);

	return B_OK;
}


/** Remove a previously installed interrupt handler */

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
	acquire_spinlock(&io_vectors[vector].vector_lock);

	/* loop through the available handlers and try to find a match.
	 * We go forward through the list but this means we start with the
	 * most recently added handlers.
	 */
	for (io = io_vectors[vector].handler_list.next;
	     io != &io_vectors[vector].handler_list;
	     io = io->next) {
		/* we have to match both function and data */
		if (io->func == handler && io->data == data) {
			remque(io);

			// Check if we need to disable the interrupt
			if (io->use_enable_counter && --io_vectors[vector].enable_count == 0)
				arch_int_disable_io_interrupt(vector);

			status = B_OK;
			break;
		}
	}

	release_spinlock(&io_vectors[vector].vector_lock);
	restore_interrupts(state);

	// if the handler could be found and removed, we still have to free it
	if (status == B_OK)
		free(io);

	return status;
} 


/** actually process an interrupt via the handlers registered for that
 *	vector (irq)
 */

int
int_io_interrupt_handler(int vector)
{
	int status = B_UNHANDLED_INTERRUPT;
	struct io_handler *io;

#if 0
{
	static int low[2];
	static int high[2];
	static int counter;
	int32 cpu = smp_get_current_cpu();

	if (vector > 0xf0 - 32)
		high[cpu]++;
	else
		low[cpu]++;
	dprintf("got vector %d at CPU %ld\n", vector, cpu);
	//if ((counter++ % 10) == 0)
		dprintf("ints: %d/%d (high: %d/%d)\n", low[0], low[1], high[0], high[1]);
}
#endif
	acquire_spinlock(&io_vectors[vector].vector_lock);

	// The list can be empty at this place
	if (io_vectors[vector].handler_list.next == &io_vectors[vector].handler_list) {
		dprintf("unhandled io interrupt %d\n", vector);
		release_spinlock(&io_vectors[vector].vector_lock);
		return B_UNHANDLED_INTERRUPT;
	}

	/* Loop through the list of handlers. 
	 * each handler returns as follows...
	 * - B_UNHANDLED_INTERRUPT, the interrupt wasn't processed by the
	 *                          fucntion, so try the next available.
	 * - B_HANDLED_INTERRUPT, the interrupt has been handled and no further
	 *                        attention is required
	 * - B_INVOKE_SCHEDULER, the interrupt has been handled, but the function wants
	 *                       the scheduler to be invoked
	 *
	 * This is a change of behaviour from newos where every handler registered
	 * be called, even if the interrupt had been "handled" by a previous
	 * function.
	 * The logic now is that if there are no handlers then we return
	 * B_UNHANDLED_INTERRUPT and let the system do as it will.
	 * When we have the first function that claims to have "handled" the
	 * interrupt, by returning B_HANDLED_... or B_INVOKE_SCHEDULER we simply
	 * stop calling further handlers and return the value from that
	 * handler.
	 * This may not be correct but appears to be what BeOS did and seems
	 * right.
	 *
	 *	ToDo: we might want to reenable calling all registered handlers depending
	 *		on a flag somewhere, so that we can deal with buggy drivers
	 */

	for (io = io_vectors[vector].handler_list.next;
		io != &io_vectors[vector].handler_list; // Are we already at the end of the list?
		io = io->next) {
		if ((status = io->func(io->data)) != B_UNHANDLED_INTERRUPT)
			break;
	}

#ifdef DEBUG_INT
	if (status != B_UNHANDLED_INTERRUPT)
		io_vectors[vector].handled_count++;
	else
		io_vectors[vector].unhandled_count++;
#endif

	release_spinlock(&io_vectors[vector].vector_lock);

	return status;
}

