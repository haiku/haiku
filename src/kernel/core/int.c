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
	int (*func)(void*); 
	void* data; 
}; 

struct io_vector { 
	struct io_handler *handler_list; 
	spinlock_t vector_lock; 
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


int
int_set_io_interrupt_handler(int vector, int (*func)(void*), void* data) 
{ 
	struct io_handler *io; 
	int state; 

	// insert this io handler in the chain of interrupt 
	// handlers registered for this io interrupt 

	io = (struct io_handler *)kmalloc(sizeof(struct io_handler)); 
	if (io == NULL) 
		return ENOMEM; 
	io->func = func; 
	io->data = data; 

	state = int_disable_interrupts(); 
	acquire_spinlock(&io_vectors[vector].vector_lock); 
	io->next = io_vectors[vector].handler_list; 
	io_vectors[vector].handler_list = io; 
	release_spinlock(&io_vectors[vector].vector_lock); 
	int_restore_interrupts(state); 

	arch_int_enable_io_interrupt(vector); 

	return 0; 
}


int
int_remove_io_interrupt_handler(int vector, int (*func)(void*), void* data) 
{ 
	struct io_handler *io, *prev = NULL; 
	int state; 

	// lock the structures down so it is not modified while we search 
	state = int_disable_interrupts(); 
	acquire_spinlock(&io_vectors[vector].vector_lock); 

	// start at the beginning 
	io = io_vectors[vector].handler_list; 

	// while not at end 
	while (io != NULL) { 
		// see if we match both the function & data 
		if (io->func == func && io->data == data) 
			break; 

		// Store our backlink and move to next 
		prev = io; 
		io = io->next; 
	} 

	// If we found it 
	if (io != NULL) { 
		// unlink it, taking care of the change it was the first in line 
		if (prev != NULL) 
			prev->next = io->next; 
		else 
			io_vectors[vector].handler_list = io->next; 
	} 

	// release our lock as we're done with the vector 
	release_spinlock(&io_vectors[vector].vector_lock); 
	int_restore_interrupts(state); 

	// and disable the IRQ if nothing left 
	if (io != NULL) { 
		if (prev == NULL && io->next == NULL) 
			arch_int_disable_io_interrupt(vector); 
	
		kfree(io); 
	} 

	return (io != NULL) ? 0 : EINVAL; 
} 


int
int_io_interrupt_handler(int vector) 
{ 
	int ret = INT_NO_RESCHEDULE; 
	
	acquire_spinlock(&io_vectors[vector].vector_lock); 
	
	if (io_vectors[vector].handler_list == NULL) { 
		dprintf("unhandled io interrupt %d\n", vector); 
	} else { 
		struct io_handler *io; 
		int temp_ret; 

		io = io_vectors[vector].handler_list; 
		while (io != NULL) { 
			temp_ret = io->func(io->data); 
			if (temp_ret == INT_RESCHEDULE) 
				ret = INT_RESCHEDULE; 
			io = io->next; 
		} 
	} 

	release_spinlock(&io_vectors[vector].vector_lock); 

	return ret; 
}

