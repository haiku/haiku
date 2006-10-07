/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "stack_private.h"
#include "utility.h"

#include <net_buffer.h>
#include <util/AutoLock.h>

#include <ByteOrder.h>
#include <KernelExport.h>


static struct list sTimers;
static benaphore sTimerLock;
static sem_id sTimerWaitSem;
static thread_id sTimerThread;


uint16
compute_checksum(uint8 *_buffer, size_t length)
{
	uint16 *buffer = (uint16 *)_buffer;
	uint32 sum = 0;

	// TODO: unfold loop for speed
	// TODO: write processor dependent version for speed
	while (length >= 2) {
		sum += *buffer++;
		length -= 2;
	}

	if (length) {
		// give the last byte it's proper endian-aware treatment
#if B_HOST_IS_LENDIAN
		sum += *(uint8 *)buffer;
#else
		uint8 ordered[2];
		ordered[0] = *(uint8 *)buffer;
		ordered[1] = 0;
		sum += *(uint16 *)ordered;
#endif
	}

	while (sum >> 16) {
		sum = (sum & 0xffff) + (sum >> 16);
	}

	return sum;
}


uint16
checksum(uint8 *buffer, size_t length)
{
	return ~compute_checksum(buffer, length);
}


//	#pragma mark - Notifications


status_t
notify_socket(net_socket *socket, uint8 event, int32 value)
{
	return gNetSocketModule.notify(socket, event, value);
}


//	#pragma mark - FIFOs


status_t
init_fifo(net_fifo *fifo, const char *name, size_t maxBytes)
{
	status_t status = benaphore_init(&fifo->lock, name);
	if (status < B_OK)
		return status;

	fifo->notify = create_sem(1, name);
	if (fifo->notify < B_OK) {
		benaphore_destroy(&fifo->lock);
		return fifo->notify;
	}

	fifo->max_bytes = maxBytes;
	fifo->current_bytes = 0;
	fifo->waiting = 0;

	list_init(&fifo->buffers);

	return B_OK;
}


void
uninit_fifo(net_fifo *fifo)
{
	clear_fifo(fifo);

	benaphore_destroy(&fifo->lock);
	delete_sem(fifo->notify);
}


status_t
fifo_enqueue_buffer(net_fifo *fifo, net_buffer *buffer)
{
	BenaphoreLocker locker(fifo->lock);

	if (fifo->max_bytes > 0 && fifo->current_bytes + buffer->size > fifo->max_bytes)
		return ENOBUFS;

	list_add_item(&fifo->buffers, buffer);
	fifo->current_bytes += buffer->size;

	if (fifo->waiting > 0) {
		fifo->waiting--;
		release_sem_etc(fifo->notify, 1, B_DO_NOT_RESCHEDULE);
			// we still hold the benaphore lock, so it makes no sense
			// to reschedule after having released the sync semaphore
	}

	return B_OK;
}


/*!
	Gets the first buffer from the FIFO. If there is no buffer, it
	will wait depending on the \a flags and \a timeout.
	The following flags are supported (the rest is ignored):
		MSG_DONTWAIT - ignores the timeout and never wait for a buffer; if your
			socket is O_NONBLOCK, you should specify this flag. A \a timeout of
			zero is equivalent to this flag, though.
		MSG_PEEK - returns a clone of the buffer and keep the original
			in the FIFO.
*/
ssize_t
fifo_dequeue_buffer(net_fifo *fifo, uint32 flags, bigtime_t timeout,
	net_buffer **_buffer)
{
	benaphore_lock(&fifo->lock);
	bool dontWait = (flags & MSG_DONTWAIT) != 0 || timeout == 0;
	status_t status;

	while (true) {
		net_buffer *buffer = (net_buffer *)list_get_first_item(&fifo->buffers);
		if (buffer != NULL) {
			if ((flags & MSG_PEEK) != 0) {
				// we need to clone the buffer for inspection; we can't give a
				// handle to a buffer that we're still using
				buffer = gNetBufferModule.clone(buffer, false);
				if (buffer == NULL) {
					status = B_NO_MEMORY;
					break;
				}
			} else
				list_remove_item(&fifo->buffers, buffer);

			*_buffer = buffer;
			status = B_OK;
			break;
		}

		if (!dontWait)
			fifo->waiting++;

		// we need to wait until a new buffer becomes available
		benaphore_unlock(&fifo->lock);

		if (dontWait)
			return B_WOULD_BLOCK;

		status = acquire_sem_etc(fifo->notify, 1,
			B_CAN_INTERRUPT | B_RELATIVE_TIMEOUT, timeout);
		if (status < B_OK)
			return status;

		// try again
		benaphore_lock(&fifo->lock);
	}

	if ((flags & MSG_PEEK) != 0 && fifo->waiting > 0) {
		// another thread is waiting for data, since we didn't eat the
		// buffer, it gets it
		fifo->waiting--;
		release_sem_etc(fifo->notify, 1, B_DO_NOT_RESCHEDULE);
	}

	benaphore_unlock(&fifo->lock);
	return status;
}


status_t
clear_fifo(net_fifo *fifo)
{
	BenaphoreLocker locker(fifo->lock);

	while (true) {
		net_buffer *buffer = (net_buffer *)list_remove_head_item(&fifo->buffers);
		if (buffer == NULL)
			break;

		gNetBufferModule.free(buffer);
	}

	fifo->current_bytes = 0;
	return B_OK;
}


//	#pragma mark - Timer


static status_t
timer_thread(void * /*data*/)
{
	status_t status = B_OK;

	do {
		bigtime_t timeout = B_INFINITE_TIMEOUT;

		if (status == B_TIMED_OUT || status == B_OK) {
			// scan timers for new timeout and/or execute a timer
			if (benaphore_lock(&sTimerLock) < B_OK)
				return B_OK;

			struct net_timer *timer = NULL;
			while (true) {
				timer = (net_timer *)list_get_next_item(&sTimers, timer);
				if (timer == NULL)
					break;

				if (timer->due < system_time()) {
					// execute timer
					list_remove_item(&sTimers, timer);
					timer->due = -1;

					benaphore_unlock(&sTimerLock);
					timer->hook(timer, timer->data);
					benaphore_lock(&sTimerLock);

					timer = NULL;
						// restart scanning as we unlocked the list
				} else {
					// calculate new timeout
					if (timer->due < timeout)
						timeout = timer->due;
				}
			}

			benaphore_unlock(&sTimerLock);
		}

		status = acquire_sem_etc(sTimerWaitSem, 1, B_ABSOLUTE_TIMEOUT, timeout);
			// the wait sem normally can't be acquired, so we
			// have to look at the status value the call returns:
			//
			// B_OK - a new timer has been added or canceled
			// B_TIMED_OUT - look for timers to be executed
			// B_BAD_SEM_ID - we are asked to quit
	} while (status != B_BAD_SEM_ID);
	
	return B_OK;
}


/*!
	Initializes a timer before use. You can also use this function to change
	a timer later on, but make sure you have canceled it before using set_timer().
*/
void
init_timer(net_timer *timer, net_timer_func hook, void *data)
{
	timer->hook = hook;
	timer->data = data;
	timer->due = 0;
}


/*!
	Sets or cancels a timer. When the \a delay is below zero, an eventually running
	timer is canceled, if not, it is scheduled to be executed after the specified
	\a delay.
	You need to have initialized the timer before calling this function.
	In case you need to change a running timer, you have to cancel it first, before
	making any changes.
*/
void
set_timer(net_timer *timer, bigtime_t delay)
{
	BenaphoreLocker locker(sTimerLock);

	if (timer->due > 0) {
		// this timer is already scheduled, cancel it
		list_remove_item(&sTimers, timer);
	}

	if (delay >= 0) {
		// add this timer
		timer->due = system_time() + delay;
		list_add_item(&sTimers, timer);
	}

	// notify timer about the change
	release_sem(sTimerWaitSem);
}


status_t
init_timers(void)
{
	list_init(&sTimers);

	status_t status = benaphore_init(&sTimerLock, "net timer");
	if (status < B_OK)
		return status;

	sTimerWaitSem = create_sem(0, "net timer wait");
	if (sTimerWaitSem < B_OK) {
		status = sTimerWaitSem;
		goto err1;
	}

	sTimerThread = spawn_kernel_thread(timer_thread, "net timer",
		B_NORMAL_PRIORITY, NULL);
	if (sTimerThread < B_OK) {
		status = sTimerThread;
		goto err2;
	}

	return resume_thread(sTimerThread);

err1:
	benaphore_destroy(&sTimerLock);
err2:
	delete_sem(sTimerWaitSem);
	return status;
}


void
uninit_timers(void)
{
	benaphore_destroy(&sTimerLock);
	delete_sem(sTimerWaitSem);

	status_t status;
	wait_for_thread(sTimerThread, &status);
}

