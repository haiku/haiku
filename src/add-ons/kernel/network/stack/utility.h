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

// timer
void		init_timer(net_timer *timer, net_timer_func hook, void *data);
void		set_timer(net_timer *timer, bigtime_t delay);
bool		cancel_timer(struct net_timer *timer);
bool		is_timer_active(net_timer *timer);
status_t	init_timers(void);
void		uninit_timers(void);

#endif	// NET_UTILITY_H
