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
//	File Name:		Invoker.cpp
//	Author:			Frans van Nispen (xlr8@tref.nl)
//	Description:	BMessageFilter class creates objects that filter
//					in-coming BMessages.  
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <Application.h>
#include <Handler.h>
#include <Invoker.h>
#include <Message.h>
#include <Messenger.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
BInvoker::BInvoker()
	:	fMessage(NULL),
		fMessenger(be_app),
		fReplyTo(NULL),
		fTimeout(B_INFINITE_TIMEOUT)
{
}
//------------------------------------------------------------------------------
BInvoker::BInvoker(BMessage* message, const BHandler* handler,
				   const BLooper* looper)
	:	fMessage(message),
		fMessenger(BMessenger(handler, looper)),
		fReplyTo(NULL),
		fTimeout(B_INFINITE_TIMEOUT)
{
}
//------------------------------------------------------------------------------
BInvoker::BInvoker(BMessage* message, BMessenger target)
	:	fMessage(message),
		fMessenger(BMessenger(target)),
		fReplyTo(NULL),
		fTimeout(B_INFINITE_TIMEOUT)
{
}
//------------------------------------------------------------------------------
BInvoker::~BInvoker()
{
	delete fMessage;
}
//------------------------------------------------------------------------------
status_t BInvoker::SetMessage(BMessage* message)
{
	delete fMessage;
	fMessage = message;
	return B_OK;
}
//------------------------------------------------------------------------------
BMessage* BInvoker::Message() const
{
	return fMessage;
}
//------------------------------------------------------------------------------
uint32 BInvoker::Command() const
{
	if (fMessage)
	{
		return fMessage->what;
	}
	else
	{
		return (uint32)NULL;
	}
}
//------------------------------------------------------------------------------
status_t BInvoker::SetTarget(const BHandler* handler, const BLooper* looper)
{
	fMessenger = BMessenger(handler, looper);
	return fMessenger.IsValid();
}
//------------------------------------------------------------------------------
status_t BInvoker::SetTarget(BMessenger messenger)
{
	fMessenger = messenger;
	return fMessenger.IsValid();
}
//------------------------------------------------------------------------------
bool BInvoker::IsTargetLocal() const
{
	return fMessenger.IsTargetLocal();
}
//------------------------------------------------------------------------------
BHandler* BInvoker::Target(BLooper** looper) const
{
	return fMessenger.Target(looper);
}
//------------------------------------------------------------------------------
BMessenger BInvoker::Messenger() const
{
	return fMessenger;
}
//------------------------------------------------------------------------------
status_t BInvoker::SetHandlerForReply(BHandler* handler)
{
	fReplyTo = handler;
	return B_OK;
}
//------------------------------------------------------------------------------
BHandler* BInvoker::HandlerForReply() const
{
	return fReplyTo;
}
//------------------------------------------------------------------------------
status_t BInvoker::Invoke(BMessage* msg)
{
	return InvokeNotify( msg );
}
//------------------------------------------------------------------------------
status_t BInvoker::InvokeNotify(BMessage* msg, uint32 kind)
{
	fNotifyKind = kind;

	BMessage clone(kind); 
	status_t err = B_BAD_VALUE;
	
	if (!msg && fNotifyKind == B_CONTROL_INVOKED)
	{
		msg = fMessage;
	}

	if (!msg)
	{
		if (!Target()->IsWatched())
			return err; 
	}
	else
	{
		clone = *msg;
	}

	clone.AddInt64("when", system_time()); 
	clone.AddPointer("source", this); 

	if (msg)
	{
		err = fMessenger.SendMessage( &clone, fReplyTo, fTimeout);
	}
	
	// Also send invocation to any observers of this handler. 
	Target()->SendNotices(kind, &clone);
	return err;
}
//------------------------------------------------------------------------------
status_t BInvoker::SetTimeout(bigtime_t timeout)
{
	fTimeout = timeout;
	return B_OK;
}
//------------------------------------------------------------------------------
bigtime_t BInvoker::Timeout() const
{
	return fTimeout;
}
//------------------------------------------------------------------------------
uint32 BInvoker::InvokeKind(bool* notify)
{
	if (fNotifyKind == B_CONTROL_INVOKED)
	{
		*notify = false;
	}
	else
	{
		*notify = true;
	}

	return fNotifyKind;
}
//------------------------------------------------------------------------------
void BInvoker::BeginInvokeNotify(uint32 kind)
{
	fNotifyKind = kind;
}
//------------------------------------------------------------------------------
void BInvoker::EndInvokeNotify()
{
	fNotifyKind = B_CONTROL_INVOKED;
}
//------------------------------------------------------------------------------
void BInvoker::_ReservedInvoker1()
{
}
//------------------------------------------------------------------------------
void BInvoker::_ReservedInvoker2()
{
}
//------------------------------------------------------------------------------
void BInvoker::_ReservedInvoker3()
{
}
//------------------------------------------------------------------------------
BInvoker::BInvoker(const BInvoker &)
{
	// No copy construction allowed!
}
//------------------------------------------------------------------------------
BInvoker& BInvoker::operator=(const BInvoker&)
{
	// No assignment allowed!
	return *this;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

