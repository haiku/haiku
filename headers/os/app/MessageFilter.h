//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		MessageFilter.h
//	Author:			Erik Jaesler (erik@cgsoftware.com)
//	Description:	BMessageFilter class creates objects that filter
//					in-coming BMessages.  
//------------------------------------------------------------------------------

#ifndef _MESSAGE_FILTER_H
#define _MESSAGE_FILTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <Handler.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class BMessage;

// filter_hook Return Codes and Protocol ---------------------------------------
enum filter_result {
	B_SKIP_MESSAGE,
	B_DISPATCH_MESSAGE
};

typedef filter_result (*filter_hook)
	(BMessage* message, BHandler** target, BMessageFilter* filter);


// BMessageFilter invocation criteria ------------------------------------------
enum message_delivery {
	B_ANY_DELIVERY,
	B_DROPPED_DELIVERY,
	B_PROGRAMMED_DELIVERY
};

enum message_source {
	B_ANY_SOURCE,
	B_REMOTE_SOURCE,
	B_LOCAL_SOURCE
};

// BMessageFilter Class --------------------------------------------------------
class BMessageFilter {

public:
								BMessageFilter(	uint32 what,
												filter_hook func = NULL);
								BMessageFilter(	message_delivery delivery,
												message_source source,
												filter_hook func = NULL);
								BMessageFilter(	message_delivery delivery,
												message_source source,
												uint32 what,
												filter_hook func = NULL);
								BMessageFilter(const BMessageFilter& filter);
								BMessageFilter(const BMessageFilter* filter);
	virtual						~BMessageFilter();

			BMessageFilter		&operator=(const BMessageFilter& from);

	// Hook function; ignored if filter_hook is non-NULL
	virtual	filter_result		Filter(BMessage* message, BHandler** target);

			message_delivery	MessageDelivery() const;
			message_source		MessageSource() const;
			uint32				Command() const;
			bool				FiltersAnyCommand() const;
			BLooper				*Looper() const;

// Private or reserved ---------------------------------------------------------
private:
	friend	class BLooper;
	friend	class BHandler;

	virtual	void				_ReservedMessageFilter1();
	virtual	void				_ReservedMessageFilter2();

			void				SetLooper(BLooper* owner);
			filter_hook			FilterFunction() const;
			bool				fFiltersAny;
			uint32				what;
			message_delivery	fDelivery;
			message_source		fSource;
			BLooper				*fLooper;
			filter_hook			fFilterFunction;
			uint32				_reserved[3];
};
//------------------------------------------------------------------------------

#endif	// _MESSAGE_FILTER_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

