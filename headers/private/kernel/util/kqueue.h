/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef KQUEUE_H
#define KQUEUE_H

/* The name "que" actually means that it is a very
 * light-weight implementation of a queue ;-))
 */

/* The object that is put into a queue must begin with these
 * fields, but it doesn't have to be this structure.
 */
struct quehead {
	struct quehead *next;
	struct quehead *prev;
};

/* You can use this macro to iterate through the queue. */
#define kqueue_foreach(head, element) \
	for ((element) = (void *)(head)->next; (element) != (void *)(head); (element) = (void *)((struct quehead *)(element))->next)


/** Initializes a queue to be used */

static inline void
initque(void *_head)
{
	struct quehead *head = (struct quehead *)_head;

	head->next = head->prev = head;
}


/** Inserts an element (_element) to the queue (_head) */

static inline void
insque(void *_element, void *_head)
{
	struct quehead *element = (struct quehead *)_element,
		 *head = (struct quehead *)_head;

	element->next = head->next;
	element->prev = head;
	head->next = element;
	element->next->prev = element;
}


/** removes an element from the queue it's currently in */

static inline void
remque(void *_element)
{
	struct quehead *element = (struct quehead *)_element;

	element->next->prev = element->prev;
	element->prev->next = element->next;
	element->prev = 0;
}

#endif	/* KQUEUE_H */
