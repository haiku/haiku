/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <kernel.h>
#include <int.h>
#include <debug.h>
#include <malloc.h>
#include <smp.h>
#include <arch/int.h>
#include <errno.h>
#include <boot/kernel_args.h>
#include <string.h>
#include <stdio.h>
#include <kqueue.h>

#define NUM_IO_VECTORS 256

struct io_handler {
	struct io_handler	*next;
	struct io_handler	*prev;
	interrupt_handler	func;
	void				*data;
};

struct io_vector {
	struct io_handler	handler_list;
	spinlock			vector_lock;
};

static struct io_vector *io_vectors = NULL;

cpu_status
disable_interrupts()
{
	return arch_int_disable_interrupts();
}


void
restore_interrupts(cpu_status status)
{
	arch_int_restore_interrupts(status);
}


int
int_init(kernel_args *ka)
{
	dprintf("init_int_handlers: entry\n");

	return arch_int_init(ka);
}


int
int_init2(kernel_args *ka)
{
	int i;
	
	io_vectors = (struct io_vector *)malloc(sizeof(struct io_vector) * NUM_IO_VECTORS);
	if (io_vectors == NULL)
		panic("int_init2: could not create io vector table!\n");

	/* initialize the vector list */
	for (i = 0; i < NUM_IO_VECTORS; i++) {
		io_vectors[i].vector_lock = 0;			/* initialize spinlock */
		initque(&io_vectors[i].handler_list);	/* initialize handler queue */
	}

	return arch_int_init2(ka);
}


/**	This function is used internally to install a handler on the given vector.
 *	NB this does NOT take an IRQ, but a system interrupt value.
 *	As this is intended for system use this function does NOT call
 *	arch_int_enable_io_interrupt() as it only works for IRQ values
 */

long
install_interrupt_handler(long vector, interrupt_handler handler, void *data)
{
	struct io_handler *io = NULL; 
	int state;

	if (vector < 0 || vector >= NUM_IO_VECTORS)
		return B_BAD_VALUE;

	/* find the chain of handlers for this irq.
	 * NB there can be multiple handlers for the same IRQ, especially for
	 * PCI drivers. Where we have multiple handlers we will call each in turn
	 * until one returns a value other than B_UNHANDLED_INTERRUPT.
	 */
	io = (struct io_handler *)malloc(sizeof(struct io_handler));
	if (io == NULL)
		return ENOMEM;

	io->func = handler;
	io->data = data;

	/* Disable the interrupts, get the spinlock for this irq only
	 * and then insert the handler */
	state = disable_interrupts();
	acquire_spinlock(&io_vectors[vector].vector_lock);

	insque(io, &io_vectors[vector].handler_list);

	release_spinlock(&io_vectors[vector].vector_lock);
	restore_interrupts(state);

	return 0;
}


/** install a handler to be called when an interrupt is triggered
 *	for the given irq with data as the argument
 */

long
install_io_interrupt_handler(long irq, interrupt_handler handler, void *data, ulong flags)
{
	long vector = irq + 0x20;
	long rv = install_interrupt_handler(vector, handler, data);

	if (rv != 0)
		return rv;

	/* If we were passed the bit-flag B_NO_ENABLE_COUNTER then
	 * we're being asked to not alter whether the interrupt is set
	 * regardless of setting.
	 */
	if ((flags & B_NO_ENABLE_COUNTER) == 0)
		arch_int_enable_io_interrupt(irq);

	return 0;
}


/**	Removes and interrupt handler.
 *	Read the notes for install_interrupt_handler!
 */

long
remove_interrupt_handler(long vector, interrupt_handler handler, void *data)
{
	struct io_handler *io = NULL;
	long status = EINVAL;
	int state;

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
			status = B_OK;
			break;
		}
	}

	/* to finish we need to release our locks and return
	 * the value rv
	 */
	release_spinlock(&io_vectors[vector].vector_lock);
	restore_interrupts(state);

	/* if the handler could be found and removed, we still have to free it */
	if (status == B_OK)
		free(io);

	return status;
} 


/** remove an interrupt handler previously inserted */

long
remove_io_interrupt_handler(long irq, interrupt_handler handler, void *data)
{
	long vector = irq + 0x20;
	long rv = remove_interrupt_handler(vector, handler, data);

	if (rv < 0)
		return rv;

	/* Check if we need to disable interrupts... */
	if (io_vectors[vector].handler_list.next != &io_vectors[vector].handler_list)
		arch_int_disable_io_interrupt(irq);

	return 0; 
} 


/** actually process an interrupt via the handlers registered for that
 *	vector (irq)
 */

int
int_io_interrupt_handler(int vector)
{
	int status = B_UNHANDLED_INTERRUPT;
	struct io_handler *io;

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

	release_spinlock(&io_vectors[vector].vector_lock);

	return status;
}

