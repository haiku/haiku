/*
 * Copyright 2006-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef NET_UTILITY_H
#define NET_UTILITY_H


#include <net_stack.h>


class UserBuffer {
public:
								UserBuffer(void* buffer, size_t size);

			void*				Push(void* source, size_t size);
			status_t			Pad(size_t length);
			status_t			PadToNext(size_t length);

			status_t			Status() const
									{ return fStatus; }
			size_t				BytesConsumed() const
									{ return fBufferSize - fAvailable; }

private:
			uint8*				fBuffer;
			size_t				fBufferSize;
			size_t				fAvailable;
			status_t			fStatus;
};


inline
UserBuffer::UserBuffer(void* buffer, size_t size)
	:
	fBuffer((uint8*)buffer),
	fBufferSize(size),
	fAvailable(size),
	fStatus(B_OK)
{
}


// checksums
uint16		compute_checksum(uint8* _buffer, size_t length);
uint16		checksum(uint8* buffer, size_t length);

// notifications
status_t	notify_socket(net_socket* socket, uint8 event, int32 value);

// fifos
status_t	init_fifo(net_fifo* fifo, const char *name, size_t maxBytes);
void		uninit_fifo(net_fifo* fifo);
status_t	fifo_enqueue_buffer(net_fifo* fifo, struct net_buffer* buffer);
ssize_t		fifo_dequeue_buffer(net_fifo* fifo, uint32 flags, bigtime_t timeout,
				struct net_buffer** _buffer);
status_t	clear_fifo(net_fifo* fifo);
status_t	fifo_socket_enqueue_buffer(net_fifo* fifo, net_socket* socket,
				uint8 event, net_buffer* buffer);

// timer
void		init_timer(net_timer* timer, net_timer_func hook, void* data);
void		set_timer(net_timer* timer, bigtime_t delay);
bool		cancel_timer(struct net_timer* timer);
status_t	wait_for_timer(struct net_timer* timer);
bool		is_timer_active(net_timer* timer);
bool		is_timer_running(net_timer* timer);
status_t	init_timers(void);
void		uninit_timers(void);

// syscall restart
bool		is_syscall(void);
bool		is_restarted_syscall(void);
void		store_syscall_restart_timeout(bigtime_t timeout);
bigtime_t	restore_syscall_restart_timeout(void);

#endif	// NET_UTILITY_H
