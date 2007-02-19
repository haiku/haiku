// SLList.h

#ifndef SL_LIST_H
#define SL_LIST_H

#include <SupportDefs.h>

namespace UserlandFSUtil {

// SLListLink
template<typename Element>
class SLListLink {
public:
	SLListLink() : next(NULL) {}
	~SLListLink() {}

	Element	*next;
};

// SLListLinkImpl
template<typename Element>
class SLListLinkImpl {
private:
	typedef SLListLink<Element> Link;

public:
	SLListLinkImpl() : fSLListLink()	{}
	~SLListLinkImpl()					{}

	Link *GetSLListLink()				{ return &fSLListLink; }
	const Link *GetSLListLink() const	{ return &fSLListLink; }

private:
	Link	fSLListLink;
};

// SLListStandardGetLink
template<typename Element>
class SLListStandardGetLink {
private:
	typedef SLListLink<Element> Link;

public:
	inline Link *operator()(Element *element) const
	{
		return element->GetSLListLink();
	}

	inline const Link *operator()(const Element *element) const
	{
		return element->GetSLListLink();
	}
};

// for convenience
#define SL_LIST_TEMPLATE_LIST template<typename Element, typename GetLink>
#define SL_LIST_CLASS_NAME SLList<Element, GetLink>

// SLList
template<typename Element, typename GetLink = SLListStandardGetLink<Element> >
class SLList {
private:
	typedef SLList<Element, GetLink>	List;
	typedef SLListLink<Element>			Link;

public:
	class Iterator {
	public:
		Iterator(List *list)
			: fList(list),
			  fPrevious(NULL),
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
			if (fCurrent)
				fPrevious = fCurrent;

			fCurrent = fNext;

			if (fNext)
				fNext = fList->GetNext(fNext);

			return fCurrent;
		}
	
		Element *Remove()
		{
			Element *element = fCurrent;
			if (fCurrent) {
				fList->_Remove(fPrevious, fCurrent);
				fCurrent = NULL;
			}
			return element;
		}

		Iterator &operator=(const Iterator &other)
		{
			fList = other.fList;
			fPrevious = other.fPrevious;
			fCurrent = other.fCurrent;
			fNext = other.fNext;
			return *this;
		}
	
	private:
		List	*fList;
		Element *fPrevious;
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
	SLList() : fFirst(NULL), fLast(NULL) {}
	SLList(const GetLink &getLink)
		: fFirst(NULL), fLast(NULL), fGetLink(getLink) {}
	~SLList() {}

	inline bool IsEmpty() const			{ return (fFirst == NULL); }

	inline void Insert(Element *element, bool back = true);
	inline void InsertAfter(Element *previous, Element *element);
	inline void Remove(Element *element);
		// O(n)!

	inline void MoveFrom(SL_LIST_CLASS_NAME *fromList);

	inline void RemoveAll();

	inline Element *GetFirst() const	{ return fFirst; }
	inline Element *GetLast() const		{ return fLast; }

	inline Element *GetHead() const		{ return fFirst; }
	inline Element *GetTail() const		{ return fLast; }

	inline Element *GetNext(Element *element) const;

	inline int32 Size() const;
		// O(n)!

	inline Iterator GetIterator()				{ return Iterator(this); }
	inline ConstIterator GetIterator() const	{ return ConstIterator(this); }

private:
	friend class Iterator;

	inline void _Remove(Element *previous, Element *element);

private:
	Element	*fFirst;
	Element	*fLast;
	GetLink	fGetLink;
};

}	// namespace UserlandFSUtil

using UserlandFSUtil::SLList;
using UserlandFSUtil::SLListLink;
using UserlandFSUtil::SLListLinkImpl;


// inline methods

// Insert
SL_LIST_TEMPLATE_LIST
void
SL_LIST_CLASS_NAME::Insert(Element *element, bool back)
{
	InsertAfter((back ? fLast : NULL), element);
}

// InsertAfter
SL_LIST_TEMPLATE_LIST
void
SL_LIST_CLASS_NAME::InsertAfter(Element *previous, Element *element)
{
	if (element) {
		Link *elLink = fGetLink(element);
		if (previous) {
			// insert after previous element
			Link *prevLink = fGetLink(previous);
			elLink->next = prevLink->next;
			prevLink->next = element;
		} else {
			// no previous element given: prepend
			elLink->next = fFirst;
			fFirst = element;
		}

		// element may be new last element
		if (fLast == previous)
			fLast = element;
	}
}

// Remove
SL_LIST_TEMPLATE_LIST
void
SL_LIST_CLASS_NAME::Remove(Element *element)
{
	if (!element)
		return;

	for (Iterator it = GetIterator(); it.HasNext();) {
		if (element == it.Next()) {
			it.Remove();
			return;
		}
	}
}

// MoveFrom
SL_LIST_TEMPLATE_LIST
void
SL_LIST_CLASS_NAME::MoveFrom(SL_LIST_CLASS_NAME *fromList)
{
	if (fromList && fromList->fFirst) {
		if (fFirst) {
			fGetLink(fLast)->next = fromList->fFirst;
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
SL_LIST_TEMPLATE_LIST
void
SL_LIST_CLASS_NAME::RemoveAll()
{
	Element *element = fFirst;
	while (element) {
		Link *elLink = fGetLink(element);
		element = elLink->next;
		elLink->next = NULL;
	}
	fFirst = NULL;
	fLast = NULL;
}

// GetNext
SL_LIST_TEMPLATE_LIST
Element *
SL_LIST_CLASS_NAME::GetNext(Element *element) const
{
	return (element ? fGetLink(element)->next : NULL);
}

// _Remove
SL_LIST_TEMPLATE_LIST
void
SL_LIST_CLASS_NAME::_Remove(Element *previous, Element *element)
{
	Link *elLink = fGetLink(element);
	if (previous)
		fGetLink(previous)->next = elLink->next;
	else
		fFirst = elLink->next;

	if (element == fLast)
		fLast = previous;

	elLink->next = NULL;
}

// Size
SL_LIST_TEMPLATE_LIST
int32
SL_LIST_CLASS_NAME::Size() const
{
	int32 count = 0;
	for (Element* element = GetFirst(); element; element = GetNext(element))
		count++;
	return count;
}

#endif	// SL_LIST_H
