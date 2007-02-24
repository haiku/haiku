// DLList.h
//
// Copyright (c) 2003, Ingo Weinhold (bonefish@cs.tu-berlin.de)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// 
// Except as contained in this notice, the name of a copyright holder shall
// not be used in advertising or otherwise to promote the sale, use or other
// dealings in this Software without prior written authorization of the
// copyright holder.

#ifndef DL_LIST_H
#define DL_LIST_H

#include <SupportDefs.h>

namespace UserlandFSUtil {

// DLListLink
template<typename Element>
class DLListLink {
public:
	DLListLink() : previous(NULL), next(NULL) {}
	~DLListLink() {}

	Element	*previous;
	Element	*next;
};

// DLListLinkImpl
template<typename Element>
class DLListLinkImpl {
private:
	typedef DLListLink<Element> MyLink;

public:
	DLListLinkImpl() : fDLListLink()	{}
	~DLListLinkImpl()					{}

	MyLink *GetDLListLink()				{ return &fDLListLink; }
	const MyLink *GetDLListLink() const	{ return &fDLListLink; }

private:
	MyLink	fDLListLink;
};

// DLListStandardGetLink
template<typename Element>
class DLListStandardGetLink {
private:
	typedef DLListLink<Element> Link;

public:
	inline Link *operator()(Element *element) const
	{
		return element->GetDLListLink();
	}

	inline const Link *operator()(const Element *element) const
	{
		return element->GetDLListLink();
	}
};

// for convenience
#define DL_LIST_TEMPLATE_LIST template<typename Element, typename GetLink>
#define DL_LIST_CLASS_NAME DLList<Element, GetLink>

// DLList
template<typename Element, typename GetLink = DLListStandardGetLink<Element> >
class DLList {
private:
	typedef DLList<Element, GetLink>	List;
	typedef DLListLink<Element>			Link;

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
	DLList() : fFirst(NULL), fLast(NULL) {}
	DLList(const GetLink &getLink)
		: fFirst(NULL), fLast(NULL), fGetLink(getLink) {}
	~DLList() {}

	inline bool IsEmpty() const			{ return (fFirst == NULL); }

	inline void Insert(Element *element, bool back = true);
	inline void Remove(Element *element);

	inline void Swap(Element *a, Element *b);

	inline void MoveFrom(DL_LIST_CLASS_NAME *fromList);

	inline void RemoveAll();

	inline Element *GetFirst() const	{ return fFirst; }
	inline Element *GetLast() const		{ return fLast; }

	inline Element *GetHead() const		{ return fFirst; }
	inline Element *GetTail() const		{ return fLast; }

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

}	// namespace UserlandFSUtil

using UserlandFSUtil::DLList;
using UserlandFSUtil::DLListLink;
using UserlandFSUtil::DLListLinkImpl;


// inline methods

// Insert
DL_LIST_TEMPLATE_LIST
void
DL_LIST_CLASS_NAME::Insert(Element *element, bool back)
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
DL_LIST_TEMPLATE_LIST
void
DL_LIST_CLASS_NAME::Remove(Element *element)
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
DL_LIST_TEMPLATE_LIST
void
DL_LIST_CLASS_NAME::Swap(Element *a, Element *b)
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
DL_LIST_TEMPLATE_LIST
void
DL_LIST_CLASS_NAME::MoveFrom(DL_LIST_CLASS_NAME *fromList)
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
DL_LIST_TEMPLATE_LIST
void
DL_LIST_CLASS_NAME::RemoveAll()
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
DL_LIST_TEMPLATE_LIST
Element *
DL_LIST_CLASS_NAME::GetPrevious(Element *element) const
{
	Element *result = NULL;
	if (element)
		result = fGetLink(element)->previous;
	return result;
}

// GetNext
DL_LIST_TEMPLATE_LIST
Element *
DL_LIST_CLASS_NAME::GetNext(Element *element) const
{
	Element *result = NULL;
	if (element)
		result = fGetLink(element)->next;
	return result;
}

// Size
DL_LIST_TEMPLATE_LIST
int32
DL_LIST_CLASS_NAME::Size() const
{
	int32 count = 0;
	for (Element* element = GetFirst(); element; element = GetNext(element))
		count++;
	return count;
}

#endif	// DL_LIST_H
