/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2005-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_UTIL_SINGLY_LINKED_LIST_H
#define KERNEL_UTIL_SINGLY_LINKED_LIST_H


#ifdef __cplusplus

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
		inline bool Remove(Element* element);
		inline void Remove(Element* previous, Element* element);

		inline void MoveFrom(SINGLY_LINKED_LIST_CLASS_NAME* fromList);
			// O(1) if either list is empty, otherwise O(n).

		inline void RemoveAll();
		inline void MakeEmpty()			{ RemoveAll(); }

		inline Element* First() const { return fFirst; }
		inline Element* Head() const { return fFirst; }

		inline Element* RemoveHead();

		inline Element* GetNext(Element* element) const;

		inline int32 Size() const;
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


/*!	Removes \a element from the list.
	It is safe to call the list with a \c NULL element or an element that isn't
	in the list.
	\param element The element to be removed.
	\return \c true, if the element was in the list and has been removed,
		\c false otherwise.
*/
SINGLY_LINKED_LIST_TEMPLATE_LIST
bool
SINGLY_LINKED_LIST_CLASS_NAME::Remove(Element* element)
{
	if (element == NULL)
		return false;

	Element* next = fFirst;
	Element* last = NULL;
	while (element != next) {
		if (next == NULL)
			return false;
		last = next;
		next = sGetLink(next)->next;
	}

	Link* elementLink = sGetLink(element);
	if (last == NULL)
		fFirst = elementLink->next;
	else
		sGetLink(last)->next = elementLink->next;

	elementLink->next = NULL;
	return true;
}


SINGLY_LINKED_LIST_TEMPLATE_LIST
void
SINGLY_LINKED_LIST_CLASS_NAME::Remove(Element* previous, Element* element)
{
//	ASSERT(previous == NULL
//		? fFirst == element : sGetLink(previous)->next == element);

	Link* elementLink = sGetLink(element);
	if (previous == NULL)
		fFirst = elementLink->next;
	else
		sGetLink(previous)->next = elementLink->next;

	elementLink->next = NULL;
}


SINGLY_LINKED_LIST_TEMPLATE_LIST
void
SINGLY_LINKED_LIST_CLASS_NAME::MoveFrom(SINGLY_LINKED_LIST_CLASS_NAME* fromList)
{
	if (fromList->fFirst == NULL)
		return;

	if (fFirst == NULL) {
		// This list is empty -- just transfer the head.
		fFirst = fromList->fFirst;
		fromList->fFirst = NULL;
		return;
	}

	// Neither list is empty -- find the tail of this list.
	Element* tail = fFirst;
	while (Element* next = sGetLink(tail)->next)
		tail = next;

	sGetLink(tail)->next = fromList->fFirst;
	fromList->fFirst = NULL;
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
int32
SINGLY_LINKED_LIST_CLASS_NAME::Size() const
{
	int32 count = 0;
	for (Element* element = First(); element; element = GetNext(element))
		count++;
	return count;
}

// sGetLink
SINGLY_LINKED_LIST_TEMPLATE_LIST
GetLink SINGLY_LINKED_LIST_CLASS_NAME::sGetLink;

#endif	/* __cplusplus */

#endif	// _KERNEL_UTIL_SINGLY_LINKED_LIST_H
