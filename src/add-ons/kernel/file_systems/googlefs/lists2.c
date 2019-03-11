/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#include <KernelExport.h>
#include <OS.h>
#include "lists2.h"

void *sll_find(long nextoff, void *head, sll_compare_func func, void *id)
{
	void *p = head;
	int count = 5000;
	if (head == NULL)
		return NULL;
	while (p) {
		if (func(p, id) == 0)
			return p;
		p = sll_next(nextoff, p);
		if (!count--) {
			dprintf("sll_find: WARNING: 5000 nodes to search ??? looks more of a loop.\n");
			return NULL;
		}
	}
	return NULL;
}

status_t sll_insert_head(long nextoff, void **head, void *item)
{
	void *next = NULL;
	if (head == NULL || item == NULL)
		return EINVAL;
	if (*head)
		next = *head;
	*(void **)(((char *)item)+nextoff) = next;
	*head = item;
	return B_OK;
}

status_t sll_insert_tail(long nextoff, void **head, void *item)
{
	void *p;
	if (head == NULL || item == NULL)
		return EINVAL;

	if (*(void **)(((char *)item)+nextoff)) {
		dprintf("sll_insert_tail: WARNING: %p->next NOT NULL\n", item);
		*(void **)(((char *)item)+nextoff) = NULL;
	}

	p = *head;
	if (!p) {
		*head = item;
		return B_OK;
	}
	while (sll_next(nextoff, p))
		p = sll_next(nextoff, p);
	*(void **)(((char *)p)+nextoff) = item;
	return B_OK;
}

void *sll_dequeue_tail(long nextoff, void **head)
{
	void **prev = NULL;
	void *curr = NULL;
	if (head == NULL || *head == NULL)
		return NULL;
	prev = head;
	curr = *head;
	while (sll_next(nextoff, curr)) {
		prev = (void **)(((char *)curr)+nextoff);
		curr = sll_next(nextoff, curr);
	}
	*prev = NULL;
	return curr;
}

status_t sll_remove(long nextoff, void **head, void *item)
{
	void **prev = NULL;
	void *curr = NULL;
	if (head == NULL || *head == NULL || item == NULL)
		return EINVAL;
	prev = head;
	curr = *head;
	while (prev && curr) {
		if (curr == item) {
			*prev = sll_next(nextoff, curr);
			*(void **)(((char *)item)+nextoff) = NULL;
			return B_OK;
		}
		prev = (void **)(((char *)curr)+nextoff);
		curr = sll_next(nextoff, curr);
	}
	return ENOENT;
}

void *sll_next(long nextoff, void *item)
{
	void *next;
	if (!item)
		return NULL;
	next = *(void **)(((char *)item)+nextoff);
	return next;
}
