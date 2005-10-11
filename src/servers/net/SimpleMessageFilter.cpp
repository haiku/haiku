/*
 * Copyright 2004, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

/*!	\class SimpleMessageFilter
	\brief This is a BMessageFilter that can filter multiple \a what values.
	
	This class extends the BMessageFilter's ability of handling one \a what value
	to handling multiple \a what values.
*/

#include "SimpleMessageFilter.h"
#include <Message.h>


/*!	\brief Creates a copy of the \a what array.
	
	\param what A pointer to an array of \a what values that should be filtered.
		The end-of-list indicator is an element valued 0 (zero).
	\param target The target for messages matching one of the \a what values.
		If \a target == NULL the messages will be discarded.
*/
SimpleMessageFilter::SimpleMessageFilter(const uint32 *what, BHandler *target)
	: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
	fTarget(target)
{
	if(!what) {
		fWhatArray = NULL;
		return;
	}
	
	// include the trailing zero in the copy
	int32 count = 0;
	while(what[count++] != 0)
		;
	fWhatArray = new uint32[count];
	memcpy(fWhatArray, what, count * sizeof(uint32));
}


//! Frees the copied \a what array.
SimpleMessageFilter::~SimpleMessageFilter()
{
	delete fWhatArray;
}


//! Filters all messages that match the \a what array given in the constructor.
filter_result
SimpleMessageFilter::Filter(BMessage *message, BHandler **target)
{
	for(int32 index = 0; fWhatArray[index] != 0; index++)
		if(fWhatArray[index] == message->what) {
			if(!fTarget)
				return B_SKIP_MESSAGE;
			
			*target = fTarget;
			break;
		}
	
	return B_DISPATCH_MESSAGE;
}
