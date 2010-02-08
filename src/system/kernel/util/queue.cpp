/* Standard queue */

/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <kernel.h>
#include <queue.h>
#include <malloc.h>
#include <errno.h>

typedef struct queue_element {
	void *next;
} queue_element;

typedef struct queue_typed {
	queue_element *head;
	queue_element *tail;
	int count;
} queue_typed;


int
queue_init(queue *q)
{
	q->head = q->tail = NULL;
	q->count = 0;
	return 0;
}


int
queue_remove_item(queue *_q, void *e)
{
	queue_typed *q = (queue_typed *)_q;
	queue_element *elem = (queue_element *)e;
	queue_element *temp, *last = NULL;

	temp = (queue_element *)q->head;
	while (temp) {
		if (temp == elem) {
			if (last)
				last->next = temp->next;
			else
				q->head = (queue_element*)temp->next;

			if (q->tail == temp)
				q->tail = last;
			q->count--;
			return 0;
		}
		last = temp;
		temp = (queue_element*)temp->next;
	}

	return -1;
}


int
queue_enqueue(queue *_q, void *e)
{
	queue_typed *q = (queue_typed *)_q;
	queue_element *elem = (queue_element *)e;

	if (q->tail == NULL) {
		q->tail = elem;
		q->head = elem;
	} else {
		q->tail->next = elem;
		q->tail = elem;
	}
	elem->next = NULL;
	q->count++;
	return 0;
}


void *
queue_dequeue(queue *_q)
{
	queue_typed *q = (queue_typed *)_q;
	queue_element *elem;

	elem = q->head;
	if (q->head != NULL)
		q->head = (queue_element*)q->head->next;
	if (q->tail == elem)
		q->tail = NULL;

	if (elem != NULL)
		q->count--;

	return elem;
}


void *
queue_peek(queue *q)
{
	return q->head;
}


//	#pragma mark -
/* fixed queue stuff */


int
fixed_queue_init(fixed_queue *q, int size)
{
	if (size <= 0)
		return EINVAL;

	q->table = (void**)malloc(size * sizeof(void *));
	if (!q->table)
		return ENOMEM;
	q->head = 0;
	q->tail = 0;
	q->count = 0;
	q->size = size;

	return 0;
}


void
fixed_queue_destroy(fixed_queue *q)
{
	free(q->table);
}


int
fixed_queue_enqueue(fixed_queue *q, void *e)
{
	if (q->count == q->size)
		return ENOMEM;

	q->table[q->head++] = e;
	if (q->head >= q->size)
		q->head = 0;
	q->count++;

	return 0;
}


void *
fixed_queue_dequeue(fixed_queue *q)
{
	void *e;

	if (q->count <= 0)
		return NULL;

	e = q->table[q->tail++];
 	if (q->tail >= q->size)
 		q->tail = 0;
	q->count--;

	return e;
}


void *
fixed_queue_peek(fixed_queue *q)
{
	if (q->count <= 0)
		return NULL;

	return q->table[q->tail];
}


