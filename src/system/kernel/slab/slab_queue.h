/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SLAB_QUEUE_H
#define SLAB_QUEUE_H


#include <stddef.h>

#include "kernel_debug_config.h"


struct slab_queue_link {
	slab_queue_link* next;
};

#if PARANOID_KERNEL_FREE
struct slab_queue {
	slab_queue_link* head;
	slab_queue_link* tail;

	void Init()
	{
		head = tail = NULL;
	}

	void Push(slab_queue_link* item)
	{
		item->next = NULL;

		if (tail == NULL) {
			head = tail = item;
			return;
		}

		tail->next = item;
		tail = item;
	}

	slab_queue_link* Pop()
	{
		slab_queue_link* item = head;
		head = item->next;
		if (head == NULL)
			tail = NULL;
		return item;
	}
};
#else /* LIFO queue */
struct slab_queue {
	slab_queue_link* head;

	void Init()
	{
		head = NULL;
	}

	void Push(slab_queue_link* item)
	{
		item->next = head;
		head = item;
	}

	slab_queue_link* Pop()
	{
		slab_queue_link* item = head;
		head = item->next;
		return item;
	}
};
#endif


#endif	// SLAB_QUEUE_H
