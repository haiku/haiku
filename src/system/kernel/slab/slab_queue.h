/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SLAB_QUEUE_H
#define SLAB_QUEUE_H


#include <stddef.h>


struct slab_queue_link {
	slab_queue_link* next;
};

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


#endif	// SLAB_QUEUE_H
