/* 
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef KERNEL_UTIL_DOUBLY_LINKED_LIST_H
#define KERNEL_UTIL_DOUBLY_LINKED_LIST_H

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
	DoublyLinkedList(const GetLink &getLink)
		: fFirst(NULL), fLast(NULL), fGetLink(getLink) {}
	~DoublyLinkedList() {}

	inline bool IsEmpty() const			{ return (fFirst == NULL); }

	inline void Insert(Element *element, bool back = true);
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
	GetLink	fGetLink;
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
			Link *elLink = fGetLink(element);
			elLink->previous = fLast;
			elLink->next = NULL;
			if (fLast)
				fGetLink(fLast)->next = element;
			else
				fFirst = element;
			fLast = element;
		} else {
			// prepend
			Link *elLink = fGetLink(element);
			elLink->previous = NULL;
			elLink->next = fFirst;
			if (fFirst)
				fGetLink(fFirst)->previous = element;
			else
				fLast = element;
			fFirst = element;
		}
	}
}

// Remove
DOUBLY_LINKED_LIST_TEMPLATE_LIST
void
DOUBLY_LINKED_LIST_CLASS_NAME::Remove(Element *element)
{
	if (element) {
		Link *elLink = fGetLink(element);
		if (elLink->previous)
			fGetLink(elLink->previous)->next = elLink->next;
		else
			fFirst = elLink->next;
		if (elLink->next)
			fGetLink(elLink->next)->previous = elLink->previous;
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
		Link *aLink = fGetLink(a);
		Link *bLink = fGetLink(b);
		Element *aPrev = aLink->previous;
		Element *bPrev = bLink->previous;
		Element *aNext = aLink->next;
		Element *bNext = bLink->next;
		// place a
		if (bPrev)
			fGetLink(bPrev)->next = a;
		else
			fFirst = a;
		if (bNext)
			fGetLink(bNext)->previous = a;
		else
			fLast = a;
		aLink->previous = bPrev;
		aLink->next = bNext;
		// place b
		if (aPrev)
			fGetLink(aPrev)->next = b;
		else
			fFirst = b;
		if (aNext)
			fGetLink(aNext)->previous = b;
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
			fGetLink(fLast)->next = fromList->fFirst;
			fGetLink(fFirst)->previous = fLast;
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
		Link *elLink = fGetLink(element);
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
		result = fGetLink(element)->previous;
	return result;
}

// GetNext
DOUBLY_LINKED_LIST_TEMPLATE_LIST
Element *
DOUBLY_LINKED_LIST_CLASS_NAME::GetNext(Element *element) const
{
	Element *result = NULL;
	if (element)
		result = fGetLink(element)->next;
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

#endif	// KERNEL_UTIL_DOUBLY_LINKED_LIST_H
