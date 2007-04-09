/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
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
	UserBuffer(void *buffer, size_t size);

	void *Copy(void *source, size_t size);
	status_t Status() const;
	size_t ConsumedAmount() const;

private:
	uint8 *fBuffer;
	size_t fBufferSize, fAvailable;
	status_t fStatus;
};


// internal Fifo class which doesn't maintain it's own lock
class Fifo {
public:
	Fifo(const char *name, size_t maxBytes);
	~Fifo();

	status_t InitCheck() const;

	status_t Enqueue(net_buffer *buffer);
	status_t EnqueueAndNotify(net_buffer *_buffer, net_socket *socket, uint8 event);
	status_t Wait(benaphore *lock, bigtime_t timeout);
	net_buffer *Dequeue(bool clone);
	status_t Clear();

	void WakeAll();

	bool IsEmpty() const { return current_bytes == 0; }

//private:
	// these field names are kept so we can use templatized
	// functions together with net_fifo
	sem_id		notify;
	int32		waiting;
	size_t		max_bytes;
	size_t		current_bytes;
	struct list	buffers;
};

inline
UserBuffer::UserBuffer(void *buffer, size_t size)
	: fBuffer((uint8 *)buffer), fBufferSize(size),
	  fAvailable(size), fStatus(B_OK)
{
}


inline status_t
UserBuffer::Status() const
{
	return fStatus;
}


inline size_t
UserBuffer::ConsumedAmount() const
{
	return fBufferSize - fAvailable;
}


// checksums
uint16		compute_checksum(uint8 *_buffer, size_t length);
uint16		checksum(uint8 *buffer, size_t length);

// notifications
status_t	notify_socket(net_socket *socket, uint8 event, int32 value);

// fifos
status_t	init_fifo(net_fifo *fifo, const char *name, size_t maxBytes);
void		uninit_fifo(net_fifo *fifo);
status_t	fifo_enqueue_buffer(net_fifo *fifo, struct net_buffer *buffer);
ssize_t		fifo_dequeue_buffer(net_fifo *fifo, uint32 flags, bigtime_t timeout,
				struct net_buffer **_buffer);
status_t	clear_fifo(net_fifo *fifo);
status_t	fifo_socket_enqueue_buffer(net_fifo *, net_socket *, uint8 event,
				net_buffer *);

// timer
void		init_timer(net_timer *timer, net_timer_func hook, void *data);
void		set_timer(net_timer *timer, bigtime_t delay);
bool		cancel_timer(struct net_timer *timer);
bool		is_timer_active(net_timer *timer);
status_t	init_timers(void);
void		uninit_timers(void);

#endif	// NET_UTILITY_H
