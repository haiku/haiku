/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef KERNEL_UTIL_DOUBLY_LINKED_LIST_H
#define KERNEL_UTIL_DOUBLY_LINKED_LIST_H


#include <SupportDefs.h>


#ifdef __cplusplus

/* This header defines a doubly-linked list. It differentiates between a link
 * and an item.
 * It's the C++ version of <util/list.h> it's very small and efficient, but
 * has not been perfectly C++-ified yet.
 * The link must be part of the item, it cannot be allocated on demand yet.
 */

namespace DoublyLinked {

class Link {
	public:
		Link *next;
		Link *prev;
};

template<class Item, Link Item::* LinkMember = &Item::fLink> class Iterator;

template<class Item, Link Item::* LinkMember = &Item::fLink>
class List {
	public:
		//typedef typename Item ValueType;
		typedef Iterator<Item, LinkMember> IteratorType;

		List()
		{
			fLink.next = fLink.prev = &fLink;
		}

		void Add(Item *item)
		{
			//Link *link = &item->*LinkMember;
			(item->*LinkMember).next = &fLink;
			(item->*LinkMember).prev = fLink.prev;

			fLink.prev->next = &(item->*LinkMember);
			fLink.prev = &(item->*LinkMember);
		}

		void Remove(Item *item)
		{
			(item->*LinkMember).next->prev = (item->*LinkMember).prev;
			(item->*LinkMember).prev->next = (item->*LinkMember).next;
		}

		Item *RemoveHead()
		{
			if (IsEmpty())
				return NULL;

			Item *item = GetItem(fLink.next);
			Remove(item);

			return item;
		}

		IteratorType Iterator()
		{
			return IteratorType(this);
		}

		bool IsEmpty()
		{
			return fLink.next == &fLink;
		}

		Link *Head()
		{
			return fLink.next;
		}


		static inline size_t Offset()
		{
			return (size_t)&(((Item *)1)->*LinkMember) - 1;
		}

		static inline Item *GetItem(Link *link)
		{
			return (Item *)((uint8 *)link - Offset());
		}

	private:
		friend class IteratorType;
		Link fLink;
};

template<class Item, Link Item::* LinkMember>
class Iterator {
	public:
		typedef List<Item, LinkMember> ListType;

		Iterator() : fCurrent(NULL) {}
		Iterator(ListType *list) : fList(list), fCurrent(list->Head()) {}

		Item *Next()
		{
			if (fCurrent == &fList->fLink)
				return NULL;

			Link *current = fCurrent;
			fCurrent = fCurrent->next;

			return ListType::GetItem(current);
		}

	private:
		ListType *fList;
		Link *fCurrent;
};

}	// namespace DoublyLinked

#endif	/* __cplusplus */

#endif	/* KERNEL_UTIL_DOUBLY_LINKED_LIST_H */
