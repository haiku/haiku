/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */
/* good old single linked list stuff */
#ifndef _LISTS_H
#define _LISTS_H

#include <stddef.h>

typedef struct _slist_entry {
	struct _slist_entry *next;
	char id[0];
} slist_entry;

typedef int (*slist_compare_func)(struct _slist_entry *item, void *id);

extern void slist_init(struct _slist_entry *item);
extern void slist_uninit(struct _slist_entry *item);
extern struct _slist_entry *slist_find(struct _slist_entry *head, slist_compare_func func, void *id);
extern status_t slist_insert_head(struct _slist_entry **head, struct _slist_entry *item);
extern struct _slist_entry *slist_dequeue_tail(struct _slist_entry **head);
extern status_t slist_remove(struct _slist_entry **head, struct _slist_entry *item);
extern struct _slist_entry *slist_next(struct _slist_entry *item);

#define LENT_TO_OBJ(_obtype, _sle, _moff) ((_obtype *)(((char *)(_sle)) - offsetof(_obtype, _moff)))
#define OBJ_TO_LENT(_obj, _member) (&((_obj)->_member))


#endif /* _LISTS_H */

