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
//	File Name:		Invoker.h
//	Author:			Frans van Nispen (xlr8@tref.nl)
//	Description:	BInvoker class defines a protocol for objects that
//					post messages to a "target".
//------------------------------------------------------------------------------

#ifndef _INVOKER_H
#define	_INVOKER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <Messenger.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class BHandler;
class BLooper;
class BMessage;

// BInvoker class --------------------------------------------------------------
class BInvoker {
public:
						BInvoker(); 
						BInvoker(BMessage *message,
							 const BHandler *handler,
							 const BLooper *looper = NULL);
						BInvoker(BMessage *message, BMessenger target);
	virtual				~BInvoker();

	virtual	status_t	SetMessage(BMessage *message);
			BMessage	*Message() const;
			uint32		Command() const;

	virtual status_t	SetTarget(const BHandler *h, const BLooper *loop = NULL);
	virtual status_t	SetTarget(BMessenger messenger);
			bool		IsTargetLocal() const;
			BHandler	*Target(BLooper **looper = NULL) const;
			BMessenger	Messenger() const;

	virtual status_t	SetHandlerForReply(BHandler *handler);
			BHandler	*HandlerForReply() const;

	virtual	status_t	Invoke(BMessage *msg = NULL);
	
			// Invoke with BHandler notification.  Use this to perform an
			// Invoke() with some other kind of notification change code.
			// (A raw invoke should always notify as B_CONTROL_INVOKED.)
			// Unlike a raw Invoke(), there is no standard message that is
			// sent.  If 'msg' is NULL, then nothing will be sent to this
			// invoker's target...  however, a notification message will
			// still be sent to any watchers of the invoker's handler.
			// Note that the BInvoker class does not actually implement
			// any of this behavior -- it is up to subclasses to override
			// Invoke() and call Notify() with the appropriate change code.
			status_t	InvokeNotify(BMessage *msg, uint32 kind = B_CONTROL_INVOKED);
			status_t	SetTimeout(bigtime_t timeout);
			bigtime_t	Timeout() const;

protected:
			// Return the change code for a notification.  This is either
			// B_CONTROL_INVOKED for raw Invoke() calls, or the kind
			// supplied to InvokeNotify().  In addition, 'notify' will be
			// set to true if this was an InvokeNotify() call, else false.
			uint32		InvokeKind(bool* notify = NULL);

			// Start and end an InvokeNotify context around an Invoke() call.
			// These are only needed for writing custom methods that
			// emulate the standard InvokeNotify() call.
			void		BeginInvokeNotify(uint32 kind = B_CONTROL_INVOKED);
			void		EndInvokeNotify();

// Private or reserved ---------------------------------------------------------
private:
	// to be able to keep binary compatibility
	virtual	void		_ReservedInvoker1();
	virtual	void		_ReservedInvoker2();
	virtual	void		_ReservedInvoker3();

						BInvoker(const BInvoker&);
			BInvoker	&operator=(const BInvoker&);

			BMessage	*fMessage;
			BMessenger	fMessenger;
			BHandler	*fReplyTo;
			bigtime_t	fTimeout;
			uint32		fNotifyKind;
			uint32		_reserved[2];	// to be able to keep binary compatibility
};
//------------------------------------------------------------------------------

#endif	// _INVOKER_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

