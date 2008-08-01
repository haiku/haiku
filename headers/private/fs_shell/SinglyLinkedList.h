/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2005-2006, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_UTIL_SINGLY_LINKED_LIST_H
#define KERNEL_UTIL_SINGLY_LINKED_LIST_H


#include "fssh_types.h"


#ifdef __cplusplus


namespace FSShell {

// SinglyLinkedListLink
template<typename Element>
class SinglyLinkedListLink {
public:
	SinglyLinkedListLink() : next(NULL) {}
	~SinglyLinkedListLink() {}

	Element* next;
};

// SinglyLinkedListLinkImpl
template<typename Element>
class SinglyLinkedListLinkImpl {
private:
	typedef SinglyLinkedListLink<Element> SLL_Link;

public:
	SinglyLinkedListLinkImpl() : fSinglyLinkedListLink() {}
	~SinglyLinkedListLinkImpl() {}

	SLL_Link* GetSinglyLinkedListLink()
		{ return &fSinglyLinkedListLink; }
	const SLL_Link* GetSinglyLinkedListLink() const
		{ return &fSinglyLinkedListLink; }

private:
	SLL_Link	fSinglyLinkedListLink;
};

// SinglyLinkedListStandardGetLink
template<typename Element>
class SinglyLinkedListStandardGetLink {
private:
	typedef SinglyLinkedListLink<Element> Link;

public:
	inline Link* operator()(Element* element) const
	{
		return element->GetSinglyLinkedListLink();
	}

	inline const Link* operator()(const Element* element) const
	{
		return element->GetSinglyLinkedListLink();
	}
};

// SinglyLinkedListMemberGetLink
template<typename Element,
	SinglyLinkedListLink<Element> Element::* LinkMember = &Element::fLink>
class SinglyLinkedListMemberGetLink {
private:
	typedef SinglyLinkedListLink<Element> Link;

public:
	inline Link* operator()(Element* element) const
	{
		return &(element->*LinkMember);
	}

	inline const Link* operator()(const Element* element) const
	{
		return &(element->*LinkMember);
	}
};


// for convenience
#define SINGLY_LINKED_LIST_TEMPLATE_LIST \
	template<typename Element, typename GetLink>
#define SINGLY_LINKED_LIST_CLASS_NAME SinglyLinkedList<Element, GetLink>


template<typename Element,
	typename GetLink = SinglyLinkedListStandardGetLink<Element> >
class SinglyLinkedList {
	private:
		typedef SinglyLinkedList<Element, GetLink> List;
		typedef SinglyLinkedListLink<Element> Link;

	public:
		class Iterator {
			public:
				Iterator(const List* list)
					:
					fList(list)
				{
					Rewind();
				}

				Iterator(const Iterator& other)
				{
					*this = other;
				}

				bool HasNext() const
				{
					return fNext;
				}

				Element* Next()
				{
					Element* element = fNext;
					if (fNext)
						fNext = fList->GetNext(fNext);
					return element;
				}

				Iterator& operator=(const Iterator& other)
				{
					fList = other.fList;
					fNext = other.fNext;
					return* this;
				}

				void Rewind()
				{
					fNext = fList->First();
				}

			private:
				const List*	fList;
				Element*	fNext;
		};

	public:
		SinglyLinkedList() : fFirst(NULL) {}
		~SinglyLinkedList() {}

		inline bool IsEmpty() const		{ return (fFirst == NULL); }

		inline void Add(Element* element);
		inline void Remove(Element* element);

		inline void RemoveAll();
		inline void MakeEmpty()			{ RemoveAll(); }

		inline Element* First() const { return fFirst; }
		inline Element* Head() const { return fFirst; }

		inline Element* RemoveHead();

		inline Element* GetNext(Element* element) const;

		inline int32_t Size() const;
			// O(n)!

		inline Iterator GetIterator() const	{ return Iterator(this); }

	private:
		Element	*fFirst;

		static GetLink	sGetLink;
};


// inline methods

// Add
SINGLY_LINKED_LIST_TEMPLATE_LIST
void
SINGLY_LINKED_LIST_CLASS_NAME::Add(Element* element)
{
	if (element != NULL) {
		sGetLink(element)->next = fFirst;
		fFirst = element;
	}
}

// Remove
SINGLY_LINKED_LIST_TEMPLATE_LIST
void
SINGLY_LINKED_LIST_CLASS_NAME::Remove(Element* element)
{
	if (element == NULL)
		return;

	Element* next = fFirst;
	Element* last = NULL;
	while (next != NULL && element != next) {
		last = next;
		next = sGetLink(next)->next;
	}

	Link* elementLink = sGetLink(element);
	if (last == NULL)
		fFirst = elementLink->next;
	else
		sGetLink(last)->next = elementLink->next;

	elementLink->next = NULL;
}

// RemoveAll
SINGLY_LINKED_LIST_TEMPLATE_LIST
void
SINGLY_LINKED_LIST_CLASS_NAME::RemoveAll()
{
	Element* element = fFirst;
	while (element) {
		Link* elLink = sGetLink(element);
		element = elLink->next;
		elLink->next = NULL;
	}
	fFirst = NULL;
}

// RemoveHead
SINGLY_LINKED_LIST_TEMPLATE_LIST
Element*
SINGLY_LINKED_LIST_CLASS_NAME::RemoveHead()
{
	Element* element = Head();
	Remove(element);
	return element;
}

// GetNext
SINGLY_LINKED_LIST_TEMPLATE_LIST
Element*
SINGLY_LINKED_LIST_CLASS_NAME::GetNext(Element* element) const
{
	Element* result = NULL;
	if (element)
		result = sGetLink(element)->next;
	return result;
}

// Size
SINGLY_LINKED_LIST_TEMPLATE_LIST
int32_t
SINGLY_LINKED_LIST_CLASS_NAME::Size() const
{
	int32_t count = 0;
	for (Element* element = First(); element; element = GetNext(element))
		count++;
	return count;
}

// sGetLink
SINGLY_LINKED_LIST_TEMPLATE_LIST
GetLink SINGLY_LINKED_LIST_CLASS_NAME::sGetLink;

}	// namespace FSShell

using FSShell::SinglyLinkedListLink;
using FSShell::SinglyLinkedListLinkImpl;
using FSShell::SinglyLinkedListStandardGetLink;
using FSShell::SinglyLinkedListMemberGetLink;
using FSShell::SinglyLinkedList;

#endif	// __cplusplus

#endif	// _KERNEL_UTIL_SINGLY_LINKED_LIST_H
