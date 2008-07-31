/* 
 * Copyright 2005-2007, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_UTIL_DOUBLY_LINKED_LIST_H
#define KERNEL_UTIL_DOUBLY_LINKED_LIST_H


#include "fssh_types.h"


#ifdef __cplusplus


namespace FSShell {


// DoublyLinkedListLink
template<typename Element>
class DoublyLinkedListLink {
public:
	DoublyLinkedListLink() : next(NULL), previous(NULL) {}
	~DoublyLinkedListLink() {}

	Element	*next;
	Element	*previous;
};

// DoublyLinkedListLinkImpl
template<typename Element>
class DoublyLinkedListLinkImpl {
private:
	typedef DoublyLinkedListLink<Element> DLL_Link;

public:
	DoublyLinkedListLinkImpl() : fDoublyLinkedListLink() {}
	~DoublyLinkedListLinkImpl() {}

	DLL_Link *GetDoublyLinkedListLink()	
		{ return &fDoublyLinkedListLink; }
	const DLL_Link *GetDoublyLinkedListLink() const
		{ return &fDoublyLinkedListLink; }

private:
	DLL_Link	fDoublyLinkedListLink;
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

// DoublyLinkedListCLink - interface to struct list
template<typename Element>
class DoublyLinkedListCLink {
	private:
		typedef DoublyLinkedListLink<Element> Link;

	public:
		inline Link *operator()(Element *element) const
		{
			return (Link *)&element->link;
		}

		inline const Link *operator()(const Element *element) const
		{
			return (const Link *)&element->link;
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
			:
			fList(list)
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
				fNext = fList->GetNext(fNext);
			return fCurrent;
		}

		Element *Current()
		{
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

		void Rewind()
		{
			fCurrent = NULL;
			fNext = fList->First();
		}

	private:
		List	*fList;
		Element	*fCurrent;
		Element	*fNext;
	};

	class ConstIterator {
	public:
		ConstIterator(const List *list)
			:
			fList(list)
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
				fNext = fList->GetNext(fNext);
			return element;
		}

		ConstIterator &operator=(const ConstIterator &other)
		{
			fList = other.fList;
			fNext = other.fNext;
			return *this;
		}

		void Rewind()
		{
			fNext = fList->First();
		}

	private:
		const List	*fList;
		Element		*fNext;
	};

	class ReverseIterator {
	public:
		ReverseIterator(List *list)
			:
			fList(list)
		{
			Rewind();
		}

		ReverseIterator(const ReverseIterator &other)
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
				fNext = fList->GetPrevious(fNext);
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

		ReverseIterator &operator=(const ReverseIterator &other)
		{
			fList = other.fList;
			fCurrent = other.fCurrent;
			fNext = other.fNext;
			return *this;
		}

		void Rewind()
		{
			fCurrent = NULL;
			fNext = fList->Last();
		}

	private:
		List	*fList;
		Element	*fCurrent;
		Element	*fNext;
	};

	class ConstReverseIterator {
	public:
		ConstReverseIterator(const List *list)
			:
			fList(list)
		{
			Rewind();
		}

		ConstReverseIterator(const ConstReverseIterator &other)
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
				fNext = fList->GetPrevious(fNext);
			return element;
		}

		ConstReverseIterator &operator=(const ConstReverseIterator &other)
		{
			fList = other.fList;
			fNext = other.fNext;
			return *this;
		}

		void Rewind()
		{
			fNext = fList->Last();
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
	inline void Insert(Element *before, Element *element);
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

	inline Element *RemoveHead();

	inline Element *GetPrevious(Element *element) const;
	inline Element *GetNext(Element *element) const;

	inline int32_t Size() const;
		// O(n)!

	inline Iterator GetIterator()				{ return Iterator(this); }
	inline ConstIterator GetIterator() const	{ return ConstIterator(this); }

	inline ReverseIterator GetReverseIterator()
		{ return ReverseIterator(this); }
	inline ConstReverseIterator GetReverseIterator() const
		{ return ConstReverseIterator(this); }

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

// Insert
DOUBLY_LINKED_LIST_TEMPLATE_LIST
void
DOUBLY_LINKED_LIST_CLASS_NAME::Insert(Element *before, Element *element)
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
		Element *aNext = sGetLink(a)->next;
		Element *bNext = sGetLink(b)->next;
		if (a == bNext) {
			Remove(a);
			Insert(b, a);
		} else if (b == aNext) {
			Remove(b);
			Insert(a, b);
		} else {
			Remove(a);
			Remove(b);
			Insert(aNext, b);
			Insert(bNext, a);
		}
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
			sGetLink(fromList->fFirst)->previous = fLast;
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

// RemoveHead
DOUBLY_LINKED_LIST_TEMPLATE_LIST
Element *
DOUBLY_LINKED_LIST_CLASS_NAME::RemoveHead()
{
	Element *element = Head();
	Remove(element);
	return element;
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
int32_t
DOUBLY_LINKED_LIST_CLASS_NAME::Size() const
{
	int32_t count = 0;
	for (Element* element = First(); element; element = GetNext(element))
		count++;
	return count;
}

// sGetLink
DOUBLY_LINKED_LIST_TEMPLATE_LIST
GetLink DOUBLY_LINKED_LIST_CLASS_NAME::sGetLink;


}	// namespace FSShell

using FSShell::DoublyLinkedListLink;
using FSShell::DoublyLinkedListLinkImpl;
using FSShell::DoublyLinkedListStandardGetLink;
using FSShell::DoublyLinkedListMemberGetLink;
using FSShell::DoublyLinkedListCLink;
using FSShell::DoublyLinkedList;


#endif	/* __cplusplus */

#endif	// _KERNEL_UTIL_DOUBLY_LINKED_LIST_H
