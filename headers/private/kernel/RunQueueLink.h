/*
 * Copyright 2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef RUN_QUEUE_LINK_H
#define RUN_QUEUE_LINK_H


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


#if KDEBUG
template<typename Element>
RunQueueLink<Element>::RunQueueLink()
	:
	fPrevious(NULL),
	fNext(NULL)
{
}
#else
template<typename Element>
RunQueueLink<Element>::RunQueueLink()
{
}
#endif


template<typename Element>
RunQueueLink<Element>*
RunQueueLinkImpl<Element>::GetRunQueueLink()
{
	return &fRunQueueLink;
}


#endif	// RUN_QUEUE_LINK_H

