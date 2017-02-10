/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the MIT License.
*/

/*
	Part of Open SCSI bus manager

	Special locks

	The only one defined herein is a spinlock that automatically
	disabled IRQs on enter and restores them on leave. Probably,
	this should be made public as it's quite basic.
*/

#ifndef __SCSI_LOCK_H__
#define __SCSI_LOCK_H__

#include <KernelExport.h>


// enhanced spinlock that automatically disables irqs when lock is hold
typedef struct spinlock_irq {
	spinlock	lock;				// normal spinlock
	cpu_status	prev_irq_state;		// irq state before spinlock was entered
} spinlock_irq;


static inline void
spinlock_irq_init(spinlock_irq *lock)
{
	B_INITIALIZE_SPINLOCK(&lock->lock);
}

static inline void
acquire_spinlock_irq(spinlock_irq *lock)
{
	cpu_status prev_irq_state = disable_interrupts();

	acquire_spinlock(&lock->lock);
	lock->prev_irq_state = prev_irq_state;
}

static inline void
release_spinlock_irq(spinlock_irq *lock)
{
	cpu_status prev_irq_state = lock->prev_irq_state;

	release_spinlock(&lock->lock);
	restore_interrupts(prev_irq_state);
}

#endif
