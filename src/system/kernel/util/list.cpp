/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <util/list.h>
#include <BytePointer.h>


#define GET_ITEM(list, item) ({ BytePointer<void> pointer((uint8*)item \
	- list->offset); &pointer; })
#define GET_LINK(list, item) ({ BytePointer<list_link> pointer((uint8*)item \
	+ list->offset); &pointer; })

/** Initializes the list with a specified offset to the link
 *	structure in the items that will be part of the list.
 */

void
list_init_etc(struct list *list, int32 offset)
{
	list->link.next = list->link.prev = &list->link;
	list->offset = offset;
}


void
list_init(struct list *list)
{
	list_init_etc(list, 0);
}


/** Adds a link to the head of the list
 */

void
list_add_link_to_head(struct list *list, void *_link)
{
	list_link *link = (list_link *)_link;

	link->next = list->link.next;
	link->prev = &list->link;

	list->link.next->prev = link;
	list->link.next = link;
}


/** Adds a link to the tail of the list
 */

void
list_add_link_to_tail(struct list *list, void *_link)
{
	list_link *link = (list_link *)_link;

	link->next = &list->link;
	link->prev = list->link.prev;

	list->link.prev->next = link;
	list->link.prev = link;
}


/** Removes a link from the list it's currently in.
 *	Note: the link has to be in a list when you call this function.
 */

void
list_remove_link(void *_link)
{
	list_link *link = (list_link *)_link;
	
	link->next->prev = link->prev;
	link->prev->next = link->next;
}


static inline list_link *
get_next_link(struct list *list, list_link *link)
{
	if (link->next == &list->link)
		return NULL;

	return link->next;
}


static inline list_link *
get_prev_link(struct list *list, list_link *link)
{
	if (link->prev == &list->link)
		return NULL;

	return link->prev;
}


/** Gets the successor for the current item. If the passed
 *	item is NULL, it returns the first entry in the list,
 *	if there is one.
 *	Returns NULL if there aren't any more items in this list.
 */

void *
list_get_next_item(struct list *list, void *item)
{
	list_link *link;

	if (item == NULL)
		return list_is_empty(list) ? NULL : GET_ITEM(list, list->link.next);

	link = get_next_link(list, GET_LINK(list, item));
	return link != NULL ? GET_ITEM(list, link) : NULL;
}


/** Gets the predecessor for the current item. If the passed
 *	item is NULL, it returns the last entry in the list,
 *	if there is one.
 *	Returns NULL if there aren't any previous items in this list.
 */

void *
list_get_prev_item(struct list *list, void *item)
{
	list_link *link;

	if (item == NULL)
		return list_is_empty(list) ? NULL : GET_ITEM(list, list->link.prev);

	link = get_prev_link(list, GET_LINK(list, item));
	return link != NULL ? GET_ITEM(list, link) : NULL;
}


void *
list_get_last_item(struct list *list)
{
	return list_is_empty(list) ? NULL : GET_ITEM(list, list->link.prev);
}


/** Adds an item to the end of the list.
 *	Similar to list_add_link_to_tail() but works on the item, not the link.
 */

void
list_add_item(struct list *list, void *item)
{
	list_add_link_to_tail(list, GET_LINK(list, item));
}


/** Removes an item from the list.
 *	Similar to list_remove_link() but works on the item, not the link.
 */

void
list_remove_item(struct list *list, void *item)
{
	list_remove_link(GET_LINK(list, item));
}


/** Inserts an item before another item in the list.
 *	If you pass NULL as \a before item, the item is added at the end of
 *	the list.
 */

void
list_insert_item_before(struct list *list, void *before, void *item)
{
	list_link *beforeLink;
	list_link *link;

	if (before == NULL) {
		list_add_item(list, item);
		return;
	}

	beforeLink = GET_LINK(list, before);
	link = GET_LINK(list, item);

	link->prev = beforeLink->prev;
	link->next = beforeLink;

	beforeLink->prev->next = link;
	beforeLink->prev = link;
}


/** Removes the first item in the list and returns it.
 *	Returns NULL if the list is empty.
 */

void *
list_remove_head_item(struct list *list)
{
	list_link *link;

	if (list_is_empty(list))
		return NULL;

	list_remove_link(link = list->link.next);
	return GET_ITEM(list, link);
}


/** Removes the last item in the list and returns it.
 *	Returns NULL if the list is empty.
 */

void *
list_remove_tail_item(struct list *list)
{
	list_link *link;

	if (list_is_empty(list))
		return NULL;

	list_remove_link(link = list->link.prev);
	return GET_ITEM(list, link);
}


/**	Moves the contents of the source list to the target list.
 *	The target list will be emptied before the items are moved;
 *	this is a very fast operation.
 */

void
list_move_to_list(struct list *sourceList, struct list *targetList)
{
	if (list_is_empty(sourceList)) {
		targetList->link.next = targetList->link.prev = &targetList->link;
		return;
	}

	*targetList = *sourceList;

	// correct link pointers to this list
	targetList->link.next->prev = &targetList->link;
	targetList->link.prev->next = &targetList->link;

	// empty source list
	sourceList->link.next = sourceList->link.prev = &sourceList->link;
}


