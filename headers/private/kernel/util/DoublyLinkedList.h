/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef KERNEL_UTIL_DOUBLY_LINKED_LIST_H
#define KERNEL_UTIL_DOUBLY_LINKED_LIST_H


#include <SupportDefs.h>
#include <util/kernel_cpp.h>


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

		static void Remove(Item *item)
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
			return IteratorType(*this);
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
		Iterator(ListType &list) : fList(&list), fCurrent(list.Head()) {}

		Item *Next()
		{
			if (fCurrent == &fList->fLink)
				return NULL;

			Link *current = fCurrent;
			fCurrent = fCurrent->next;

			return ListType::GetItem(current);
		}

	private:
		ListType	*fList;
		Link		*fCurrent;
};

}	// namespace DoublyLinked

#endif	/* __cplusplus */

#endif	/* KERNEL_UTIL_DOUBLY_LINKED_LIST_H */



/* 
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _KERNEL_UTIL_DOUBLY_LINKED_LIST_H
#define _KERNEL_UTIL_DOUBLY_LINKED_LIST_H

#ifdef __cplusplus

#include <SupportDefs.h>

// DoublyLinkedListLink
template<typename Element>
class DoublyLinkedListLink {
public:
	DoublyLinkedListLink() : previous(NULL), next(NULL) {}
	~DoublyLinkedListLink() {}

	Element	*previous;
	Element	*next;
};

// DoublyLinkedListLinkImpl
template<typename Element>
class DoublyLinkedListLinkImpl {
private:
	typedef DoublyLinkedListLink<Element> Link;

public:
	DoublyLinkedListLinkImpl() : fDoublyLinkedListLink() {}
	~DoublyLinkedListLinkImpl() {}

	Link *GetDoublyLinkedListLink()	
		{ return &fDoublyLinkedListLink; }
	const Link *GetDoublyLinkedListLink() const
		{ return &fDoublyLinkedListLink; }

private:
	Link	fDoublyLinkedListLink;
};

// DoublyLinkedListStandardGetLink
template<typename Element>
class DoublyLinkedListStandardGetLink {
private:
	typedef DoublyLinkedListLink<Element> Link;

public:
	inline Link *operator()(Element *element) const
	{
		return element->GetDoublyLinkedListLink();
	}

	inline const Link *operator()(const Element *element) const
	{
		return element->GetDoublyLinkedListLink();
	}
};

// DoublyLinkedListMemberGetLink
template<typename Element,
	DoublyLinkedListLink<Element> Element::* LinkMember = &Element::fLink>
class DoublyLinkedListMemberGetLink {
private:
	typedef DoublyLinkedListLink<Element> Link;

public:
	inline Link *operator()(Element *element) const
	{
		return &(element->*LinkMember);
	}

	inline const Link *operator()(const Element *element) const
	{
		return &(element->*LinkMember);
	}
};

// for convenience
#define DOUBLY_LINKED_LIST_TEMPLATE_LIST \
	template<typename Element, typename GetLink>
#define DOUBLY_LINKED_LIST_CLASS_NAME DoublyLinkedList<Element, GetLink>

// DoublyLinkedList
template<typename Element,
	typename GetLink = DoublyLinkedListStandardGetLink<Element> >
class DoublyLinkedList {
private:
	typedef DoublyLinkedList<Element, GetLink>	List;
	typedef DoublyLinkedListLink<Element>		Link;

public:
	class Iterator {
	public:
		Iterator(List *list)
			: fList(list),
			  fCurrent(NULL),
			  fNext(fList->GetFirst())
		{
		}

		Iterator(const Iterator &other)
		{
			*this = other;
		}

		bool HasNext() const
		{
			return fNext;
		}
		
		Element *Next()
		{
			fCurrent = fNext;
			if (fNext)
				fNext = fList->GetNext(fNext);
			return fCurrent;
		}
	
		Element *Remove()
		{
			Element *element = fCurrent;
			if (fCurrent) {
				fList->Remove(fCurrent);
				fCurrent = NULL;
			}
			return element;
		}

		Iterator &operator=(const Iterator &other)
		{
			fList = other.fList;
			fCurrent = other.fCurrent;
			fNext = other.fNext;
			return *this;
		}
	
	private:
		List	*fList;
		Element	*fCurrent;
		Element	*fNext;
	};

	class ConstIterator {
	public:
		ConstIterator(const List *list)
			: fList(list),
			  fNext(list->GetFirst())
		{
		}

		ConstIterator(const ConstIterator &other)
		{
			*this = other;
		}

		bool HasNext() const
		{
			return fNext;
		}
		
		Element *Next()
		{
			Element *element = fNext;
			if (fNext)
				fNext = fList->GetNext(fNext);
			return element;
		}
	
		ConstIterator &operator=(const ConstIterator &other)
		{
			fList = other.fList;
			fNext = other.fNext;
			return *this;
		}
	
	private:
		const List	*fList;
		Element		*fNext;
	};

public:
	DoublyLinkedList() : fFirst(NULL), fLast(NULL) {}
	~DoublyLinkedList() {}

	inline bool IsEmpty() const			{ return (fFirst == NULL); }

	inline void Insert(Element *element, bool back = true);
	inline void Add(Element *element, bool back = true);
	inline void Remove(Element *element);

	inline void Swap(Element *a, Element *b);

	inline void MoveFrom(DOUBLY_LINKED_LIST_CLASS_NAME *fromList);

	inline void RemoveAll();
	inline void MakeEmpty()				{ RemoveAll(); }

	inline Element *First() const		{ return fFirst; }
	inline Element *Last() const		{ return fLast; }

	inline Element *Head() const		{ return fFirst; }
	inline Element *Tail() const		{ return fLast; }

	inline Element *GetPrevious(Element *element) const;
	inline Element *GetNext(Element *element) const;

	inline int32 Size() const;
		// O(n)!

	inline Iterator GetIterator()				{ return Iterator(this); }
	inline ConstIterator GetIterator() const	{ return ConstIterator(this); }

private:
	Element	*fFirst;
	Element	*fLast;

	static GetLink	sGetLink;
};


// inline methods

// Insert
DOUBLY_LINKED_LIST_TEMPLATE_LIST
void
DOUBLY_LINKED_LIST_CLASS_NAME::Insert(Element *element, bool back)
{
	if (element) {
		if (back) {
			// append
			Link *elLink = sGetLink(element);
			elLink->previous = fLast;
			elLink->next = NULL;
			if (fLast)
				sGetLink(fLast)->next = element;
			else
				fFirst = element;
			fLast = element;
		} else {
			// prepend
			Link *elLink = sGetLink(element);
			elLink->previous = NULL;
			elLink->next = fFirst;
			if (fFirst)
				sGetLink(fFirst)->previous = element;
			else
				fLast = element;
			fFirst = element;
		}
	}
}

// Add
DOUBLY_LINKED_LIST_TEMPLATE_LIST
void
DOUBLY_LINKED_LIST_CLASS_NAME::Add(Element *element, bool back)
{
	Insert(element, back);
}

// Remove
DOUBLY_LINKED_LIST_TEMPLATE_LIST
void
DOUBLY_LINKED_LIST_CLASS_NAME::Remove(Element *element)
{
	if (element) {
		Link *elLink = sGetLink(element);
		if (elLink->previous)
			sGetLink(elLink->previous)->next = elLink->next;
		else
			fFirst = elLink->next;
		if (elLink->next)
			sGetLink(elLink->next)->previous = elLink->previous;
		else
			fLast = elLink->previous;
		elLink->previous = NULL;
		elLink->next = NULL;
	}
}

// Swap
DOUBLY_LINKED_LIST_TEMPLATE_LIST
void
DOUBLY_LINKED_LIST_CLASS_NAME::Swap(Element *a, Element *b)
{
	if (a && b && a != b) {
		Link *aLink = sGetLink(a);
		Link *bLink = sGetLink(b);
		Element *aPrev = aLink->previous;
		Element *bPrev = bLink->previous;
		Element *aNext = aLink->next;
		Element *bNext = bLink->next;
		// place a
		if (bPrev)
			sGetLink(bPrev)->next = a;
		else
			fFirst = a;
		if (bNext)
			sGetLink(bNext)->previous = a;
		else
			fLast = a;
		aLink->previous = bPrev;
		aLink->next = bNext;
		// place b
		if (aPrev)
			sGetLink(aPrev)->next = b;
		else
			fFirst = b;
		if (aNext)
			sGetLink(aNext)->previous = b;
		else
			fLast = b;
		bLink->previous = aPrev;
		bLink->next = aNext;
	}
}

// MoveFrom
DOUBLY_LINKED_LIST_TEMPLATE_LIST
void
DOUBLY_LINKED_LIST_CLASS_NAME::MoveFrom(DOUBLY_LINKED_LIST_CLASS_NAME *fromList)
{
	if (fromList && fromList->fFirst) {
		if (fFirst) {
			sGetLink(fLast)->next = fromList->fFirst;
			sGetLink(fFirst)->previous = fLast;
			fLast = fromList->fLast;
		} else {
			fFirst = fromList->fFirst;
			fLast = fromList->fLast;
		}
		fromList->fFirst = NULL;
		fromList->fLast = NULL;
	}
}

// RemoveAll
DOUBLY_LINKED_LIST_TEMPLATE_LIST
void
DOUBLY_LINKED_LIST_CLASS_NAME::RemoveAll()
{
	Element *element = fFirst;
	while (element) {
		Link *elLink = sGetLink(element);
		element = elLink->next;
		elLink->previous = NULL;
		elLink->next = NULL;
	}
	fFirst = NULL;
	fLast = NULL;
}

// GetPrevious
DOUBLY_LINKED_LIST_TEMPLATE_LIST
Element *
DOUBLY_LINKED_LIST_CLASS_NAME::GetPrevious(Element *element) const
{
	Element *result = NULL;
	if (element)
		result = sGetLink(element)->previous;
	return result;
}

// GetNext
DOUBLY_LINKED_LIST_TEMPLATE_LIST
Element *
DOUBLY_LINKED_LIST_CLASS_NAME::GetNext(Element *element) const
{
	Element *result = NULL;
	if (element)
		result = sGetLink(element)->next;
	return result;
}

// Size
DOUBLY_LINKED_LIST_TEMPLATE_LIST
int32
DOUBLY_LINKED_LIST_CLASS_NAME::Size() const
{
	int32 count = 0;
	for (Element* element = GetFirst(); element; element = GetNext(element))
		count++;
	return count;
}

// sGetLink
DOUBLY_LINKED_LIST_TEMPLATE_LIST
GetLink DOUBLY_LINKED_LIST_CLASS_NAME::sGetLink;

#endif	/* __cplusplus */

#endif	// _KERNEL_UTIL_DOUBLY_LINKED_LIST_H
