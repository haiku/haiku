/*
 * Copyright 2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef KERNEL_UTIL_HEAP_H
#define KERNEL_UTIL_HEAP_H


#include <debug.h>

#include <SupportDefs.h>


template<typename Element, typename Key>
struct HeapLink {
						HeapLink();

			int			fIndex;
			Key			fKey;
};

template<typename Element, typename Key>
class HeapLinkImpl {
private:
	typedef HeapLink<Element, Key> Link;

public:
	inline	Link*		GetHeapLink();

private:
			Link		fHeapLink;
};

template<typename Element, typename Key>
class HeapStandardGetLink {
private:
	typedef HeapLink<Element, Key> Link;

public:
	inline	Link*		operator()(Element* element) const;
};

template<typename Element, typename Key,
	HeapLink<Element, Key> Element::*LinkMember>
class HeapMemberGetLink {
private:
	typedef HeapLink<Element, Key> Link;

public:
	inline	Link*		operator()(Element* element) const;
};

template<typename Key>
class HeapLesserCompare {
public:
	inline	bool		operator()(Key a, Key b);
};

template<typename Key>
class HeapGreaterCompare {
public:
	inline	bool		operator()(Key a, Key b);
};

#define HEAP_TEMPLATE_LIST	\
	template<typename Element, typename Key, typename Compare, typename GetLink>
#define HEAP_CLASS_NAME	Heap<Element, Key, Compare, GetLink>

template<typename Element, typename Key,
	typename Compare = HeapLesserCompare<Key>,
	typename GetLink = HeapStandardGetLink<Element, Key> >
class Heap {
public:
						Heap();
						~Heap();

	inline	Element*	PeekRoot();

	inline	void		ModifyKey(Element* element, Key newKey);

	inline	void		RemoveRoot();
	inline	status_t	Insert(Element* element, Key key);

private:
			status_t	_GrowHeap();

			void		_MoveUp(HeapLink<Element, Key>* link);
			void		_MoveDown(HeapLink<Element, Key>* link);

			Element**	fElements;
			int			fLastElement;
			int			fSize;

	Compare		sCompare;
	GetLink		sGetLink;

};


#if KDEBUG
template<typename Element, typename Key>
HeapLink<Element, Key>::HeapLink()
	:
	fIndex(-1)
{
}
#else
template<typename Element, typename Key>
HeapLink<Element, Key>::HeapLink()
{
}
#endif


template<typename Element, typename Key>
HeapLink<Element, Key>*
HeapLinkImpl<Element, Key>::GetHeapLink()
{
	return &fHeapLink;
}


template<typename Element, typename Key>
HeapLink<Element, Key>*
HeapStandardGetLink<Element, Key>::operator()(Element* element) const
{
	return element->GetHeapLink();
}


template<typename Element, typename Key,
	HeapLink<Element, Key> Element::*LinkMember>
HeapLink<Element, Key>*
HeapMemberGetLink<Element, Key, LinkMember>::operator()(Element* element) const
{
	return &(element->*LinkMember);
}


template<typename Key>
bool
HeapLesserCompare<Key>::operator()(Key a, Key b)
{
	return a < b;
}


template<typename Key>
bool
HeapGreaterCompare<Key>::operator()(Key a, Key b)
{
	return a > b;
}


HEAP_TEMPLATE_LIST
HEAP_CLASS_NAME::Heap()
	:
	fElements(NULL),
	fLastElement(0),
	fSize(0)
{
}


HEAP_TEMPLATE_LIST
HEAP_CLASS_NAME::~Heap()
{
	free(fElements);
}


HEAP_TEMPLATE_LIST
Element*
HEAP_CLASS_NAME::PeekRoot()
{
	if (fLastElement > 0)
		return fElements[0];
	return NULL;
}


HEAP_TEMPLATE_LIST
void
HEAP_CLASS_NAME::ModifyKey(Element* element, Key newKey)
{
	HeapLink<Element, Key> link = sGetLink(element);

	ASSERT(link->fIndex >= 0 && link->fIndex < fLastElement);
	Key oldKey = link->fKey;
	link->fKey = newKey;

	if (sCompare(newKey, oldKey))
		_MoveUp(link);
	else
		_MoveDown(link);
}


HEAP_TEMPLATE_LIST
void
HEAP_CLASS_NAME::RemoveRoot()
{
	ASSERT(fLastElement > 0);

#if KDEBUG
	Element* element = PeekRoot();
	HeapLink<Element, Key> link = sGetLink(element);
	link->fIndex = -1;
#endif

	fLastElement--;
	if (fLastElement > 0) {
		Element* lastElement = fElements[fLastElement];
		fElements[0] = lastElement;
		sGetLink(lastElement)->fIndex = 0;
		_MoveDown(sGetLink(lastElement));
	}
}


HEAP_TEMPLATE_LIST
status_t
HEAP_CLASS_NAME::Insert(Element* element, Key key)
{
	if (fLastElement == fSize) {
		status_t result = _GrowHeap();
		if (result != B_OK)
			return result;
	}

	ASSERT(fLastElement != fSize);

	HeapLink<Element, Key>* link = sGetLink(element);

	fElements[fLastElement] = element;
	link->fIndex = fLastElement++;
	link->fKey = key;
	_MoveUp(link);
}


HEAP_TEMPLATE_LIST
status_t
HEAP_CLASS_NAME::_GrowHeap()
{
	int newSize = max_c(fSize * 2, 4);

	size_t arraySize = newSize * sizeof(Element*);
	Element** newBuffer
		= reinterpret_cast<Element**>(realloc(fElements, arraySize));
	if (newBuffer == NULL)
		return B_NO_MEMORY;

	fElements = newBuffer;
	fSize = newSize;

	return B_OK;
}


HEAP_TEMPLATE_LIST
void
HEAP_CLASS_NAME::_MoveUp(HeapLink<Element, Key>* link)
{
	int i = link->fIndex;
	while (true) {
		int parent = (link->fIndex - 1) / 2;
		if (link->fIndex > 0
			&& sCompare(link->fKey, sGetLink(fElements[parent])->fKey)) {

			sGetLink(fElements[parent])->fIndex = link->fIndex;

			Element* element = fElements[link->fIndex];
			fElements[link->fIndex] = fElements[parent];
			fElements[parent] = element;

			link->fIndex = parent;
		} else
			break;
	}
}


HEAP_TEMPLATE_LIST
void
HEAP_CLASS_NAME::_MoveDown(HeapLink<Element, Key>* link)
{
	int current;

	while (true) {
		current = link->fIndex;

		int child = 2 * link->fIndex + 1;
		if (child < fLastElement
			&& sCompare(sGetLink(fElements[child])->fKey, link->fKey)) {
			current = child;
		}

		child = 2 * link->fIndex + 2;
		if (child < fLastElement
			&& sCompare(sGetLink(fElements[child])->fKey,
				sGetLink(fElements[current])->fKey)) {
			current = child;
		}

		if (link->fIndex == current)
			break;

		sGetLink(fElements[current])->fIndex = link->fIndex;

		Element* element = fElements[link->fIndex];
		fElements[link->fIndex] = fElements[current];
		fElements[current] = element;

		link->fIndex = current;
	}
}


#endif	// KERNEL_UTIL_HEAP_H

