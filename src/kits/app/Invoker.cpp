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
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BInvoker class defines a protocol for objects that
//					post messages to a "target".
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Invoker.h>
#include <Errors.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
BInvoker::BInvoker(BMessage *message, BMessenger messenger)
	:	fMessage(message),
		fMessenger(messenger),
		fReplyTo(NULL),
		fTimeout(B_INFINITE_TIMEOUT),
		fNotifyKind(0)
{	
}
//------------------------------------------------------------------------------
BInvoker::BInvoker(BMessage *message, const BHandler *handler,
				   const BLooper *looper)
	:	fMessage(message),
		fMessenger(BMessenger(handler, looper)),
		fReplyTo(NULL),
		fTimeout(B_INFINITE_TIMEOUT),
		fNotifyKind(0)
{
}
//------------------------------------------------------------------------------
BInvoker::BInvoker()
	:	fMessage(NULL),
		fReplyTo(NULL),
		fTimeout(B_INFINITE_TIMEOUT),
		fNotifyKind(0)
{
}
//------------------------------------------------------------------------------
BInvoker::~BInvoker()
{
	delete fMessage;
}
//------------------------------------------------------------------------------
status_t BInvoker::SetMessage(BMessage *message)
{
	if (fMessage == message)
		return B_OK;

	if (fMessage)
		delete fMessage;

	fMessage = message;

	return B_OK;
}
//------------------------------------------------------------------------------
BMessage *BInvoker::Message() const
{
	return fMessage;
}
//------------------------------------------------------------------------------
uint32 BInvoker::Command() const
{
	if (fMessage)
		return fMessage->what;
	else
		return 0;
}
//------------------------------------------------------------------------------
status_t BInvoker::SetTarget(BMessenger messenger)
{
	fMessenger = messenger;

	return B_OK;
}
//------------------------------------------------------------------------------
status_t BInvoker::SetTarget(const BHandler *handler, const BLooper *looper)
{
	fMessenger = BMessenger(handler, looper);

	return B_OK;
}
//------------------------------------------------------------------------------
bool BInvoker::IsTargetLocal() const
{
	return fMessenger.IsTargetLocal();
}
//------------------------------------------------------------------------------
BHandler *BInvoker::Target(BLooper **looper) const
{
	return fMessenger.Target(looper);
}
//------------------------------------------------------------------------------
BMessenger BInvoker::Messenger() const
{
	return fMessenger;
}
//------------------------------------------------------------------------------
status_t BInvoker::SetHandlerForReply(BHandler *replyHandler)
{
	fReplyTo = replyHandler;

	return B_OK;
}
//------------------------------------------------------------------------------
BHandler *BInvoker::HandlerForReply() const
{
	return fReplyTo;
}
//------------------------------------------------------------------------------
status_t BInvoker::Invoke(BMessage *message)
{
	if (!message)
		message = Message();

	if (!message)
		return B_BAD_VALUE;

	return fMessenger.SendMessage(message, fReplyTo, fTimeout);
}
//------------------------------------------------------------------------------
status_t BInvoker::InvokeNotify(BMessage *message, uint32 kind)
{
	if (fNotifyKind != 0)
		return B_WOULD_BLOCK;

	BeginInvokeNotify(kind);

	status_t err = Invoke(message);

	EndInvokeNotify();

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
uint32 BInvoker::InvokeKind(bool *notify)
{
	if (notify)
		*notify = (fNotifyKind != 0) ? true : false;

	if (fNotifyKind != 0)
		return fNotifyKind;
	else
		return B_CONTROL_INVOKED;
}
//------------------------------------------------------------------------------
void BInvoker::BeginInvokeNotify(uint32 kind)
{
	fNotifyKind = kind;
}
//------------------------------------------------------------------------------
void BInvoker::EndInvokeNotify()
{
	fNotifyKind = 0;
}
//------------------------------------------------------------------------------
void BInvoker::_ReservedInvoker1() {}
void BInvoker::_ReservedInvoker2() {}
void BInvoker::_ReservedInvoker3() {}
//------------------------------------------------------------------------------
BInvoker::BInvoker(const BInvoker &)
{
}
//------------------------------------------------------------------------------
BInvoker &BInvoker::operator=(const BInvoker &)
{
	return *this;
}
//------------------------------------------------------------------------------
