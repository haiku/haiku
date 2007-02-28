/* 
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2005-2006, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_UTIL_DOUBLY_LINKED_QUEUE_H
#define KERNEL_UTIL_DOUBLY_LINKED_QUEUE_H


#include <util/DoublyLinkedList.h>


/*!
	A doubly linked queue is like a doubly linked list, but only has a pointer
	to the head of the list, none to its tail.
*/


#ifdef __cplusplus

// for convenience
#define DOUBLY_LINKED_QUEUE_CLASS_NAME DoublyLinkedQueue<Element, GetLink>


template<typename Element,
	typename GetLink = DoublyLinkedListStandardGetLink<Element> >
class DoublyLinkedQueue {
	private:
		typedef DoublyLinkedQueue<Element, GetLink>	Queue;
		typedef DoublyLinkedListLink<Element>		Link;

	public:
		class Iterator {
			public:
				Iterator(Queue *queue)
					:
					fQueue(queue)
				{
					Rewind();
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
						fNext = fQueue->GetNext(fNext);
					return fCurrent;
				}

				Element *Remove()
				{
					Element *element = fCurrent;
					if (fCurrent) {
						fQueue->Remove(fCurrent);
						fCurrent = NULL;
					}
					return element;
				}

				Iterator &operator=(const Iterator &other)
				{
					fQueue = other.fQueue;
					fCurrent = other.fCurrent;
					fNext = other.fNext;
					return *this;
				}

				void Rewind()
				{
					fCurrent = NULL;
					fNext = fQueue->First();
				}

			private:
				Queue	*fQueue;
				Element	*fCurrent;
				Element	*fNext;
		};

		class ConstIterator {
			public:
				ConstIterator(const Queue *queue)
					:
					fQueue(queue)
				{
					Rewind();
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
						fNext = fQueue->GetNext(fNext);
					return element;
				}

				ConstIterator &operator=(const ConstIterator &other)
				{
					fQueue = other.fQueue;
					fNext = other.fNext;
					return *this;
				}

				void Rewind()
				{
					fNext = fQueue->First();
				}

			private:
				const Queue	*fQueue;
				Element		*fNext;
		};

	public:
		DoublyLinkedQueue() : fFirst(NULL) {}
		~DoublyLinkedQueue() {}

		inline bool IsEmpty() const		{ return (fFirst == NULL); }

		inline void Insert(Element *element);
		inline void Insert(Element *before, Element *element);
		inline void Add(Element *element);
		inline void Remove(Element *element);

		inline void Swap(Element *a, Element *b);

		inline void MoveFrom(DOUBLY_LINKED_QUEUE_CLASS_NAME *fromList);

		inline void RemoveAll();
		inline void MakeEmpty()			{ RemoveAll(); }

		inline Element *First() const	{ return fFirst; }
		inline Element *Head() const	{ return fFirst; }

		inline Element *RemoveHead();

		inline Element *GetPrevious(Element *element) const;
		inline Element *GetNext(Element *element) const;

		inline int32 Size() const;
			// O(n)!

		inline Iterator GetIterator()				{ return Iterator(this); }
		inline ConstIterator GetIterator() const	{ return ConstIterator(this); }

	private:
		Element	*fFirst;
	
		static GetLink	sGetLink;
};


// inline methods

// Insert
DOUBLY_LINKED_LIST_TEMPLATE_LIST
void
DOUBLY_LINKED_QUEUE_CLASS_NAME::Insert(Element *element)
{
	if (element) {
		Link *elLink = sGetLink(element);
		elLink->previous = NULL;
		elLink->next = fFirst;
		if (fFirst)
			sGetLink(fFirst)->previous = element;
		fFirst = element;
	}
}

// Insert
DOUBLY_LINKED_LIST_TEMPLATE_LIST
void
DOUBLY_LINKED_QUEUE_CLASS_NAME::Insert(Element *before, Element *element)
{
	if (before == NULL) {
		Insert(element);
		return;
	}
	if (element == NULL)
		return;

	Link *beforeLink = sGetLink(before);
	Link *link = sGetLink(element);

	link->next = before;
	link->previous = beforeLink->previous;
	if (link->previous != NULL)
		sGetLink(link->previous)->next = element;
	beforeLink->previous = element;

	if (fFirst == before)
		fFirst = element;
}

// Add
DOUBLY_LINKED_LIST_TEMPLATE_LIST
void
DOUBLY_LINKED_QUEUE_CLASS_NAME::Add(Element *element)
{
	Insert(element);
}

// Remove
DOUBLY_LINKED_LIST_TEMPLATE_LIST
void
DOUBLY_LINKED_QUEUE_CLASS_NAME::Remove(Element *element)
{
	if (element) {
		Link *elLink = sGetLink(element);
		if (elLink->previous)
			sGetLink(elLink->previous)->next = elLink->next;
		else
			fFirst = elLink->next;
		if (elLink->next)
			sGetLink(elLink->next)->previous = elLink->previous;

		elLink->previous = NULL;
		elLink->next = NULL;
	}
}

// Swap
DOUBLY_LINKED_LIST_TEMPLATE_LIST
void
DOUBLY_LINKED_QUEUE_CLASS_NAME::Swap(Element *a, Element *b)
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

		aLink->previous = bPrev;
		aLink->next = bNext;
		// place b
		if (aPrev)
			sGetLink(aPrev)->next = b;
		else
			fFirst = b;
		if (aNext)
			sGetLink(aNext)->previous = b;

		bLink->previous = aPrev;
		bLink->next = aNext;
	}
}

// MoveFrom
DOUBLY_LINKED_LIST_TEMPLATE_LIST
void
DOUBLY_LINKED_QUEUE_CLASS_NAME::MoveFrom(DOUBLY_LINKED_QUEUE_CLASS_NAME *fromList)
{
	if (fromList && fromList->fFirst) {
		if (fFirst) {
			Element *element = fFirst;
			Link *elLink;
			while ((elLink = sGetLink(element))->next) {
				element = elLink->next;
			}
			elLink->next = fromList->fFirst;
		} else {
			fFirst = fromList->fFirst;
		}
		fromList->fFirst = NULL;
	}
}

// RemoveAll
DOUBLY_LINKED_LIST_TEMPLATE_LIST
void
DOUBLY_LINKED_QUEUE_CLASS_NAME::RemoveAll()
{
	Element *element = fFirst;
	while (element) {
		Link *elLink = sGetLink(element);
		element = elLink->next;
		elLink->previous = NULL;
		elLink->next = NULL;
	}
	fFirst = NULL;
}

// RemoveHead
DOUBLY_LINKED_LIST_TEMPLATE_LIST
Element *
DOUBLY_LINKED_QUEUE_CLASS_NAME::RemoveHead()
{
	Element *element = Head();
	Remove(element);
	return element;
}

// GetPrevious
DOUBLY_LINKED_LIST_TEMPLATE_LIST
Element *
DOUBLY_LINKED_QUEUE_CLASS_NAME::GetPrevious(Element *element) const
{
	Element *result = NULL;
	if (element)
		result = sGetLink(element)->previous;
	return result;
}

// GetNext
DOUBLY_LINKED_LIST_TEMPLATE_LIST
Element *
DOUBLY_LINKED_QUEUE_CLASS_NAME::GetNext(Element *element) const
{
	Element *result = NULL;
	if (element)
		result = sGetLink(element)->next;
	return result;
}

// Size
DOUBLY_LINKED_LIST_TEMPLATE_LIST
int32
DOUBLY_LINKED_QUEUE_CLASS_NAME::Size() const
{
	int32 count = 0;
	for (Element* element = First(); element; element = GetNext(element))
		count++;
	return count;
}

// sGetLink
DOUBLY_LINKED_LIST_TEMPLATE_LIST
GetLink DOUBLY_LINKED_QUEUE_CLASS_NAME::sGetLink;

#endif	/* __cplusplus */

#endif	// _KERNEL_UTIL_DOUBLY_LINKED_QUEUE_H
