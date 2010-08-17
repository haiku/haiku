/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 */


#include <Invoker.h>


BInvoker::BInvoker(BMessage* message, BMessenger messenger)
	:
	fMessage(message),
	fMessenger(messenger),
	fReplyTo(NULL),
	fTimeout(B_INFINITE_TIMEOUT),
	fNotifyKind(0)
{
}


BInvoker::BInvoker(BMessage* message, const BHandler* handler,
	const BLooper* looper)
	:
	fMessage(message),
	fMessenger(BMessenger(handler, looper)),
	fReplyTo(NULL),
	fTimeout(B_INFINITE_TIMEOUT),
	fNotifyKind(0)
{
}


BInvoker::BInvoker()
	:
	fMessage(NULL),
	fReplyTo(NULL),
	fTimeout(B_INFINITE_TIMEOUT),
	fNotifyKind(0)
{
}


BInvoker::~BInvoker()
{
	delete fMessage;
}


status_t
BInvoker::SetMessage(BMessage* message)
{
	if (fMessage == message)
		return B_OK;

	delete fMessage;
	fMessage = message;

	return B_OK;
}


BMessage*
BInvoker::Message() const
{
	return fMessage;
}


uint32
BInvoker::Command() const
{
	if (fMessage)
		return fMessage->what;

	return 0;
}


status_t
BInvoker::SetTarget(BMessenger messenger)
{
	fMessenger = messenger;
	return B_OK;
}


status_t
BInvoker::SetTarget(const BHandler* handler, const BLooper* looper)
{
	fMessenger = BMessenger(handler, looper);
	return B_OK;
}


bool
BInvoker::IsTargetLocal() const
{
	return fMessenger.IsTargetLocal();
}


BHandler*
BInvoker::Target(BLooper** _looper) const
{
	return fMessenger.Target(_looper);
}


BMessenger
BInvoker::Messenger() const
{
	return fMessenger;
}


status_t
BInvoker::SetHandlerForReply(BHandler* replyHandler)
{
	fReplyTo = replyHandler;
	return B_OK;
}


BHandler*
BInvoker::HandlerForReply() const
{
	return fReplyTo;
}


status_t
BInvoker::Invoke(BMessage* message)
{
	if (!message)
		message = Message();

	if (!message)
		return B_BAD_VALUE;

	return fMessenger.SendMessage(message, fReplyTo, fTimeout);
}


status_t
BInvoker::InvokeNotify(BMessage* message, uint32 kind)
{
	if (fNotifyKind != 0)
		return B_WOULD_BLOCK;

	BeginInvokeNotify(kind);
	status_t err = Invoke(message);
	EndInvokeNotify();

	return err;
}


status_t
BInvoker::SetTimeout(bigtime_t timeout)
{
	fTimeout = timeout;
	return B_OK;
}


bigtime_t
BInvoker::Timeout() const
{
	return fTimeout;
}


uint32
BInvoker::InvokeKind(bool* _notify)
{
	if (_notify)
		*_notify = fNotifyKind != 0;

	if (fNotifyKind != 0)
		return fNotifyKind;

	return B_CONTROL_INVOKED;
}


void
BInvoker::BeginInvokeNotify(uint32 kind)
{
	fNotifyKind = kind;
}


void
BInvoker::EndInvokeNotify()
{
	fNotifyKind = 0;
}


void BInvoker::_ReservedInvoker1() {}
void BInvoker::_ReservedInvoker2() {}
void BInvoker::_ReservedInvoker3() {}


BInvoker::BInvoker(const BInvoker &)
{
}


BInvoker &
BInvoker::operator=(const BInvoker &)
{
	return *this;
}

