/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef KQUEUE_H
#define KQUEUE_H


/* The object that is put into a queue must begin with these
 * fields, but it doesn't have to be this structure.
 */
struct quehead {
	struct quehead *next;
	struct quehead *prev;
};


/** Inserts an element (a) to the queue (b) */

static inline void
insque(void *a, void *b)
{
	struct quehead *element = (struct quehead *)a,
		 *head = (struct quehead *)b;

	element->next = head->next;
	element->prev = head;
	head->next = element;
	element->next->prev = element;
}


/** removes an element from the queue it's currently in */

static inline void
remque(void *a)
{
	struct quehead *element = (struct quehead *)a;

	element->next->prev = element->prev;
	element->prev->next = element->next;
	element->prev = 0;
}

#endif	/* KQUEUE_H */
