/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved. 
** Distributed under the terms of the NewOS License. 
*/ 

#include <kernel.h> 
#include <int.h> 
#include <debug.h> 
#include <memheap.h> 
#include <smp.h> 
#include <arch/int.h> 
#include <errno.h> 
#include <stage2.h> 
#include <string.h> 
#include <stdio.h> 

#define NUM_IO_VECTORS 256 

struct io_handler { 
	struct io_handler *next;
	struct io_handler *prev;
	interrupt_handler func; 
	void* data; 
}; 

struct io_vector { 
	struct io_handler handler_list; 
	spinlock_t        vector_lock; 
}; 

static struct io_vector *io_vectors = NULL;

int
int_init(kernel_args *ka) 
{ 
	dprintf("init_int_handlers: entry\n"); 

	return arch_int_init(ka); 
} 


int
int_init2(kernel_args *ka) 
{ 
	io_vectors = (struct io_vector *)kmalloc(sizeof(struct io_vectors *) * NUM_IO_VECTORS); 
	if (io_vectors == NULL) 
		panic("int_init2: could not create io vector table!\n"); 

	memset(io_vectors, 0, sizeof(struct io_vector *) * NUM_IO_VECTORS); 

	return arch_int_init2(ka); 
} 


/* install_io_interrupt_handler
 * install a handler to be called when an interrupt is triggered
 * for the given irq with data as the argument
 */
long install_io_interrupt_handler(long irq, interrupt_handler handler,
                                 void* data, ulong flags) 
{ 
	struct io_handler *io = NULL; 
	int state; 

	/* find the chain of handlers for this irq.
	 * NB there can be multiple handlers for the same IRQ, especially for
	 * PCI drivers. Where we have multiple handlers we will call each in turn
	 * until one returns a value other than B_UNHANDLED_INTERRUPT.
	 */
	io = (struct io_handler *)kmalloc(sizeof(struct io_handler)); 
	if (io == NULL) 
		return ENOMEM; 
	io->func = handler; 
	io->data = data; 

	/* Make sure our list is init'd or bad things will happen */
	if (io_vectors[irq].handler_list.next == NULL) {
		io_vectors[irq].handler_list.next = &io_vectors[irq].handler_list;
		io_vectors[irq].handler_list.prev = &io_vectors[irq].handler_list;
	}
	
	/* Disable the interrupts, get the spinlock for this irq only
	 * and then insert the handler */
	state = int_disable_interrupts();
	acquire_spinlock(&io_vectors[irq].vector_lock); 
	insque(io, &io_vectors[irq].handler_list);
	release_spinlock(&io_vectors[irq].vector_lock); 
	int_restore_interrupts(state); 

	/* If we were passed the bit-flag B_NO_ENABLE_COUNTER then 
	 * we're being asked to not alter whether the interrupt is set
	 * regardless of setting.
	 */
	if ((flags & B_NO_ENABLE_COUNTER) == 0)
		arch_int_enable_io_interrupt(irq); 

	return 0; 
}

/* remove_io_interrupt_handler
 * remove an interrupt handler previously inserted
 */
long remove_io_interrupt_handler(long irq, interrupt_handler handler, 
                                     void* data) 
{ 
	struct io_handler *io = NULL; 
	int state; 

	// lock the structures down so it is not modified while we search 
	state = int_disable_interrupts(); 
	acquire_spinlock(&io_vectors[irq].vector_lock); 

	/* loop through the available handlers and try to find a match.
	 * We go forward through the list but this means we start with the 
	 * most recently added handlers.
	 */
	io = io_vectors[irq].handler_list.next; 
	while (io != &io_vectors[irq].handler_list) { 
		/* we have to match both function and data */ 
		if (io->func == handler && io->data == data) 
			break; 
		io = io->next; 
	} 

	if (io) 
		remque(io);
	
	// release our lock as we're done with the vector 
	release_spinlock(&io_vectors[irq].vector_lock); 
	int_restore_interrupts(state); 

	// and disable the IRQ if nothing left 
	if (io != NULL) { 
		/* we still have handlers left if the next handler doesn't point back
		 * to the head of the list.
		 */
		if (io_vectors[irq].handler_list.next != &io_vectors[irq].handler_list) 
			arch_int_disable_io_interrupt(irq); 
	
		kfree(io); 
	} 

	return (io != NULL) ? 0 : EINVAL; 
} 

/* int_io_interrupt_handler
 * actually process an interrupt via the handlers registered for that
 * vector (irq)
 */
int int_io_interrupt_handler(int vector) 
{ 
	int ret = B_UNHANDLED_INTERRUPT; 
	
	acquire_spinlock(&io_vectors[vector].vector_lock); 
	
	if (io_vectors[vector].handler_list.next == &io_vectors[vector].handler_list) { 
		dprintf("unhandled io interrupt %d\n", vector); 
	} else { 
		struct io_handler *io; 
		/* Loop through the list of handlers. 
		 * each handler returns as follows...
		 * - B_UNHANDLED_INTERRUPT, the interrupt wasn't processed by the
		 *                          fucntion, so try the next available.
		 * - B_HANDLED_INTERRUPT, the interrupt has been handled and no further
		 *                        attention is required
		 * - B_INVOKE_SCHEDULER, the interrupt has been handled, but the function wants
		 *                       the scheduler to be invoked
		 */
		for (io = io_vectors[vector].handler_list.next; 
		     io != &io_vectors[vector].handler_list;
		     io = io->next) {
			if ((ret = io->func(io->data)) != B_UNHANDLED_INTERRUPT)
				break;
		} 
	} 

	release_spinlock(&io_vectors[vector].vector_lock); 

	return ret; 
}

