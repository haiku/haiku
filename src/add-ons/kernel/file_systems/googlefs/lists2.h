/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */
/* good old single linked list stuff */
#ifndef _LISTS2_H
#define _LISTS2_H

typedef int (*sll_compare_func)(const void *item, void *id);

extern void *sll_find(long nextoff, void *head, sll_compare_func func, void *id);
extern status_t sll_insert_head(long nextoff, void **head, void *item);
extern status_t sll_insert_tail(long nextoff, void **head, void *item);
extern void *sll_dequeue_tail(long nextoff, void **head);
extern status_t sll_remove(long nextoff, void **head, void *item);
extern void *sll_next(long nextoff, void *item);

#define SLLPTROFF(type,item,nextp) ((typeof(item))(((char *)(item)) + offsetof(item, nextp)))
#define SLLITEM2PTR(type,item,nextp) ((typeof(item))(((char *)(item)) + offsetof(item, nextp)))
#define SLLPTR2ITEM(type,ptr,nextp) ((typeof(ptr))(((char *)(ptr)) - offsetof(ptr, nextp)))
#define SLLNEXT(type,item,nextp) (*(typeof(item))(((char *)(item)) + offsetof(item, nextp)))

#define SLL_FIND(_head,_nextp,_func,_with) (typeof(_head))sll_find(offsetof(typeof(*_head),_nextp), (void *)(_head), _func, _with)
#define SLL_INSERT(_head,_nextp,_item) sll_insert_head(offsetof(typeof(*_head),_nextp), (void **)&(_head), _item)
//#define SLL_INSERT_TAIL(_head,_nextp,_item) (typeof(_head))sll_insert_tail(offsetof(typeof(*_head),_nextp), (void **)&(_head), _item)
#define SLL_INSERT_TAIL(_head,_nextp,_item) sll_insert_head(offsetof(typeof(*_head),_nextp), (void **)&(_head), _item)
#define SLL_DEQUEUE(_head,_nextp) (typeof(_head))sll_dequeue_tail(offsetof(typeof(*_head),_nextp), (void **)&(_head))
#define SLL_REMOVE(_head,_nextp,_item) sll_remove(offsetof(typeof(*_head),_nextp), (void **)&(_head), _item)

#endif /* _LISTS2_H */
