// SMInvoker.cpp

#include "SMInvoker.h"
#include "SMMessages.h"

// SMInvoker

// constructor
SMInvoker::SMInvoker()
		 : fReplyMessage(NULL)
{
}

// destructor
SMInvoker::~SMInvoker()
{
	delete fReplyMessage;
}

// ReplySuccess
bool
SMInvoker::ReplySuccess()
{
	return (fReplyMessage && fReplyMessage->what == MSG_REPLY);
}

// DirectReply
bool
SMInvoker::DirectReply()
{
	return fReplyMessage;
}


// SMInvoker1

// constructor
SMInvoker1::SMInvoker1(bool useReplyTo)
		  : SMInvoker(),
			fUseReplyTo(useReplyTo)
{
}

// Invoke
status_t
SMInvoker1::Invoke(BMessenger &target, BHandler *replyHandler,
				   BMessenger &replyMessenger)
{
	BHandler *replyTo = (fUseReplyTo ? replyHandler : NULL);
	status_t result = target.SendMessage(MSG_TEST, replyTo);
	return result;
}


// SMInvoker2

// constructor
SMInvoker2::SMInvoker2(bool useMessage, bool useReplyTo, bigtime_t timeout)
		  : SMInvoker(),
			fUseMessage(useMessage),
			fUseReplyTo(useReplyTo),
			fTimeout(timeout)
{
}

// Invoke
status_t
SMInvoker2::Invoke(BMessenger &target, BHandler *replyHandler,
				   BMessenger &replyMessenger)
{
	BHandler *replyTo = (fUseReplyTo ? replyHandler : NULL);
	BMessage _message(MSG_TEST);
	BMessage *message = (fUseMessage ? &_message : NULL);
	status_t result = target.SendMessage(message, replyTo, fTimeout);
	return result;
}


// SMInvoker3

// constructor
SMInvoker3::SMInvoker3(bool useMessage, bool useReplyTo, bigtime_t timeout)
		  : SMInvoker(),
			fUseMessage(useMessage),
			fUseReplyTo(useReplyTo),
			fTimeout(timeout)
{
}

// Invoke
status_t
SMInvoker3::Invoke(BMessenger &target, BHandler *replyHandler,
				   BMessenger &replyMessenger)
{
	BMessenger badMessenger;
	BMessenger &replyTo = (fUseReplyTo ? replyMessenger : badMessenger);
	BMessage _message(MSG_TEST);
	BMessage *message = (fUseMessage ? &_message : NULL);
	status_t result = target.SendMessage(message, replyTo, fTimeout);
	return result;
}


// SMInvoker4

// constructor
SMInvoker4::SMInvoker4(bool useReply)
		  : SMInvoker(),
			fUseReply(useReply)
{
}

// Invoke
status_t
SMInvoker4::Invoke(BMessenger &target, BHandler *replyHandler,
				   BMessenger &replyMessenger)
{
	if (fUseReply)
		fReplyMessage = new BMessage(0UL);
	status_t result = target.SendMessage(MSG_TEST, fReplyMessage);
	return result;
}


// SMInvoker5

// constructor
SMInvoker5::SMInvoker5(bool useMessage, bool useReply,
					   bigtime_t deliveryTimeout, bigtime_t replyTimeout)
		  : SMInvoker(),
			fUseMessage(useMessage),
			fUseReply(useReply),
			fDeliveryTimeout(deliveryTimeout),
			fReplyTimeout(replyTimeout)
{
}

// Invoke
status_t
SMInvoker5::Invoke(BMessenger &target, BHandler *replyHandler,
				   BMessenger &replyMessenger)
{
	if (fUseReply)
		fReplyMessage = new BMessage(0UL);
	BMessage _message(MSG_TEST);
	BMessage *message = (fUseMessage ? &_message : NULL);
	status_t result = target.SendMessage(message, fReplyMessage,
										 fDeliveryTimeout, fReplyTimeout);
	return result;
}


