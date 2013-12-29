/*
 * Copyright 2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef RUN_QUEUE_H
#define RUN_QUEUE_H


#include <util/Heap.h>

#include "scheduler_profiler.h"


template<typename Element>
struct RunQueueLink {
					RunQueueLink();

	unsigned int	fPriority;
	Element*		fPrevious;
	Element*		fNext;
};

template<typename Element>
class RunQueueLinkImpl {
public:
	inline	RunQueueLink<Element>*	GetRunQueueLink();

private:
			RunQueueLink<Element>	fRunQueueLink;
};

template<typename Element>
class RunQueueStandardGetLink {
private:
	typedef RunQueueLink<Element> Link;

public:
	inline	Link*		operator()(Element* element) const;
};

template<typename Element, RunQueueLink<Element> Element::*LinkMember>
class RunQueueMemberGetLink {
private:
	typedef RunQueueLink<Element> Link;

public:
	inline	Link*		operator()(Element* element) const;
};

#define RUN_QUEUE_TEMPLATE_LIST	\
	template<typename Element, unsigned int MaxPriority, typename GetLink>
#define RUN_QUEUE_CLASS_NAME	RunQueue<Element, MaxPriority, GetLink>

template<typename Element, unsigned int MaxPriority,
	typename GetLink = RunQueueStandardGetLink<Element> >
class RunQueue {
public:
	class ConstIterator {
	public:
								ConstIterator();
								ConstIterator(const RunQueue<Element,
										MaxPriority, GetLink>* list);

		inline	ConstIterator&	operator=(const ConstIterator& other);

				bool			HasNext() const;
				Element*		Next();

				void			Rewind();

	private:
		inline	void			_FindNextPriority();

				const RUN_QUEUE_CLASS_NAME*	fList;
				unsigned int	fPriority;
				Element*		fNext;

		static	GetLink			sGetLink;
	};

						RunQueue();

	inline	status_t	GetInitStatus();

	inline	Element*	PeekMaximum() const;

	inline	void		PushFront(Element* element, unsigned int priority);
	inline	void		PushBack(Element* elementt, unsigned int priority);

	inline	void		Remove(Element* element);

	inline	Element*	GetHead(unsigned int priority) const;

	inline	ConstIterator	GetConstIterator() const;

private:
	struct PriorityEntry : public HeapLinkImpl<PriorityEntry, unsigned int>
	{
	};

	typedef Heap<PriorityEntry, unsigned int, HeapGreaterCompare<unsigned int> >
		PriorityHeap;

			status_t	fInitStatus;

			PriorityEntry	fPriorityEntries[MaxPriority + 1];
			PriorityHeap	fPriorityHeap;

			Element*	fHeads[MaxPriority + 1];
			Element*	fTails[MaxPriority + 1];

	static	GetLink		sGetLink;
};


template<typename Element>
RunQueueLink<Element>::RunQueueLink()
	:
	fPrevious(NULL),
	fNext(NULL)
{
}


template<typename Element>
RunQueueLink<Element>*
RunQueueLinkImpl<Element>::GetRunQueueLink()
{
	return &fRunQueueLink;
}


template<typename Element>
RunQueueLink<Element>*
RunQueueStandardGetLink<Element>::operator()(Element* element) const
{
	return element->GetRunQueueLink();
}


template<typename Element, RunQueueLink<Element> Element::*LinkMember>
RunQueueLink<Element>*
RunQueueMemberGetLink<Element, LinkMember>::operator()(Element* element) const
{
	return &(element->*LinkMember);
}


RUN_QUEUE_TEMPLATE_LIST
RUN_QUEUE_CLASS_NAME::ConstIterator::ConstIterator()
	:
	fList(NULL)
{
}


RUN_QUEUE_TEMPLATE_LIST
RUN_QUEUE_CLASS_NAME::ConstIterator::ConstIterator(const RunQueue<Element,
		MaxPriority, GetLink>* list)
	:
	fList(list)
{
	Rewind();
}


RUN_QUEUE_TEMPLATE_LIST
typename RUN_QUEUE_CLASS_NAME::ConstIterator&
RUN_QUEUE_CLASS_NAME::ConstIterator::operator=(const ConstIterator& other)
{
	fList = other.fList;
	fPriority = other.fPriority;
	fNext = other.fNext;

	return *this;
}


RUN_QUEUE_TEMPLATE_LIST
bool
RUN_QUEUE_CLASS_NAME::ConstIterator::HasNext() const
{
	return fNext != NULL;
}


RUN_QUEUE_TEMPLATE_LIST
Element*
RUN_QUEUE_CLASS_NAME::ConstIterator::Next()
{
	ASSERT(HasNext());

	Element* current = fNext;
	RunQueueLink<Element>* link = sGetLink(fNext);

	fNext = link->fNext;
	if (fNext == NULL)
		_FindNextPriority();

	return current;
}


RUN_QUEUE_TEMPLATE_LIST
void
RUN_QUEUE_CLASS_NAME::ConstIterator::Rewind()
{
	ASSERT(fList != NULL);

	fPriority = MaxPriority;
	fNext = fList->GetHead(fPriority);
	if (fNext == NULL)
		_FindNextPriority();
}


RUN_QUEUE_TEMPLATE_LIST
void
RUN_QUEUE_CLASS_NAME::ConstIterator::_FindNextPriority()
{
	ASSERT(fList != NULL);

	while (fPriority-- > 0) {
		fNext = fList->GetHead(fPriority);
		if (fNext != NULL)
			break;
	}
}


RUN_QUEUE_TEMPLATE_LIST
RUN_QUEUE_CLASS_NAME::RunQueue()
	:
	fInitStatus(B_OK),
	fPriorityHeap(MaxPriority + 1)
{
	memset(fHeads, 0, sizeof(fHeads));
	memset(fTails, 0, sizeof(fTails));
}


RUN_QUEUE_TEMPLATE_LIST
status_t
RUN_QUEUE_CLASS_NAME::GetInitStatus()
{
	return fInitStatus;
}


RUN_QUEUE_TEMPLATE_LIST
Element*
RUN_QUEUE_CLASS_NAME::PeekMaximum() const
{
	SCHEDULER_ENTER_FUNCTION();

	PriorityEntry* maxPriority = fPriorityHeap.PeekRoot();
	if (maxPriority == NULL)
		return NULL;
	unsigned int priority = PriorityHeap::GetKey(maxPriority);

	ASSERT(priority <= MaxPriority);
	ASSERT(fHeads[priority] != NULL);

	Element* element = fHeads[priority];

	ASSERT(sGetLink(element)->fPriority == priority);
	ASSERT(fTails[priority] != NULL);
	ASSERT(sGetLink(element)->fPrevious == NULL);

	return element;
}


RUN_QUEUE_TEMPLATE_LIST
void
RUN_QUEUE_CLASS_NAME::PushFront(Element* element,
	unsigned int priority)
{
	SCHEDULER_ENTER_FUNCTION();

	ASSERT(priority <= MaxPriority);

	RunQueueLink<Element>* elementLink = sGetLink(element);

	ASSERT(elementLink->fPrevious == NULL);
	ASSERT(elementLink->fNext == NULL);

	ASSERT((fHeads[priority] == NULL && fTails[priority] == NULL)
		|| (fHeads[priority] != NULL && fTails[priority] != NULL));

	elementLink->fPriority = priority;
	elementLink->fNext = fHeads[priority];
	if (fHeads[priority] != NULL)
		sGetLink(fHeads[priority])->fPrevious = element;
	else {
		fTails[priority] = element;
		fPriorityHeap.Insert(&fPriorityEntries[priority], priority);
	}
	fHeads[priority] = element;
}


RUN_QUEUE_TEMPLATE_LIST
void
RUN_QUEUE_CLASS_NAME::PushBack(Element* element,
	unsigned int priority)
{
	SCHEDULER_ENTER_FUNCTION();

	ASSERT(priority <= MaxPriority);

	RunQueueLink<Element>* elementLink = sGetLink(element);

	ASSERT(elementLink->fPrevious == NULL);
	ASSERT(elementLink->fNext == NULL);

	ASSERT((fHeads[priority] == NULL && fTails[priority] == NULL)
		|| (fHeads[priority] != NULL && fTails[priority] != NULL));

	elementLink->fPriority = priority;
	elementLink->fPrevious = fTails[priority];
	if (fTails[priority] != NULL)
		sGetLink(fTails[priority])->fNext = element;
	else {
		fHeads[priority] = element;
		fPriorityHeap.Insert(&fPriorityEntries[priority], priority);
	}
	fTails[priority] = element;
}


RUN_QUEUE_TEMPLATE_LIST
void
RUN_QUEUE_CLASS_NAME::Remove(Element* element)
{
	SCHEDULER_ENTER_FUNCTION();

	RunQueueLink<Element>* elementLink = sGetLink(element);
	unsigned int priority = elementLink->fPriority;

	ASSERT(elementLink->fPrevious != NULL || fHeads[priority] == element);
	ASSERT(elementLink->fNext != NULL || fTails[priority] == element);

	if (elementLink->fPrevious != NULL)
		sGetLink(elementLink->fPrevious)->fNext = elementLink->fNext;
	else
		fHeads[priority] = elementLink->fNext;
	if (elementLink->fNext != NULL)
		sGetLink(elementLink->fNext)->fPrevious = elementLink->fPrevious;
	else
		fTails[priority] = elementLink->fPrevious;

	ASSERT((fHeads[priority] == NULL && fTails[priority] == NULL)
		|| (fHeads[priority] != NULL && fTails[priority] != NULL));

	if (fHeads[priority] == NULL) {
		fPriorityHeap.ModifyKey(&fPriorityEntries[priority], MaxPriority + 1);
		ASSERT(fPriorityHeap.PeekRoot() == &fPriorityEntries[priority]);
		fPriorityHeap.RemoveRoot();
	}

	elementLink->fPrevious = NULL;
	elementLink->fNext = NULL;
}


RUN_QUEUE_TEMPLATE_LIST
Element*
RUN_QUEUE_CLASS_NAME::GetHead(unsigned int priority) const
{
	SCHEDULER_ENTER_FUNCTION();

	ASSERT(priority <= MaxPriority);
	return fHeads[priority];
}


RUN_QUEUE_TEMPLATE_LIST
typename RUN_QUEUE_CLASS_NAME::ConstIterator
RUN_QUEUE_CLASS_NAME::GetConstIterator() const
{
	return ConstIterator(this);
}


RUN_QUEUE_TEMPLATE_LIST
GetLink RUN_QUEUE_CLASS_NAME::sGetLink;

RUN_QUEUE_TEMPLATE_LIST
GetLink RUN_QUEUE_CLASS_NAME::ConstIterator::sGetLink;


#endif	// RUN_QUEUE_H

