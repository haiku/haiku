/*
 * Copyright 2004, Waldemar Kornewald <Waldemar.Kornewald@web.de>
 * Distributed under the terms of the MIT License.
 */

#ifndef _SIMPLE_MESSAGE_FILTER__H
#define _SIMPLE_MESSAGE_FILTER__H

#include <MessageFilter.h>


class SimpleMessageFilter : public BMessageFilter {
	public:
		SimpleMessageFilter(const uint32 *what, BHandler *target);
		virtual ~SimpleMessageFilter();
		
		virtual filter_result Filter(BMessage *message, BHandler **target);

	private:
		uint32 *fWhatArray;
		BHandler *fTarget;
};


#endif
