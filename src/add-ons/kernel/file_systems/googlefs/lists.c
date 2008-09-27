/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>
#include "lists.h"

void slist_init(struct _slist_entry *item)
{
	item->next = NULL;
}

void slist_uninit(struct _slist_entry *item)
{
	item->next = NULL;
}

struct _slist_entry *slist_find(struct _slist_entry *head, slist_compare_func func, void *id)
{
	struct _slist_entry *p = head;
	if (head == NULL)
		return NULL;
	while (p) {
		if (func(p, id) == 0)
			return p;
		p = p->next;
	}
	return NULL;
}

status_t slist_insert_head(struct _slist_entry **head, struct _slist_entry *item)
{
	struct _slist_entry *next = NULL;
	if (head == NULL || item == NULL)
		return EINVAL;
	if (*head)
		next = *head;
	item->next = next;
	*head = item;
	return B_OK;
}

struct _slist_entry *slist_dequeue_tail(struct _slist_entry **head)
{
	struct _slist_entry **prev = NULL;
	struct _slist_entry *curr = NULL;
	if (head == NULL || *head == NULL)
		return NULL;
	prev = head;
	curr = *head;
	while (curr->next) {
		prev = &(curr->next);
		curr = curr->next;
	}
	*prev = NULL;
	return curr;
}

status_t slist_remove(struct _slist_entry **head, struct _slist_entry *item)
{
	struct _slist_entry **prev = NULL;
	struct _slist_entry *curr = NULL;
	if (head == NULL || *head == NULL || item == NULL)
		return EINVAL;
	prev = head;
	curr = *head;
	while (prev && curr) {
		if (curr == item) {
			*prev = curr->next;
			curr->next = NULL;
			return B_OK;
		}
		prev = &(curr->next);
		curr = curr->next;
	}
	return ENOENT;
}

struct _slist_entry *slist_next(struct _slist_entry *item)
{
	if (!item || !item->next)
		return NULL;
	return item->next;
}
