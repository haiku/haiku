// SMLooper.cpp

#include <Message.h>

#include "SMLooper.h"
#include "SMMessages.h"

// SMLooper

// constructor
SMLooper::SMLooper()
		: BLooper(NULL, B_NORMAL_PRIORITY, 1),
		  fUnblockTime(0),
		  fReplyDelay(0),
		  fDeliveryTime(-1),
		  fReplyTime(-1)
{
}

// destructor
SMLooper::~SMLooper()
{
}

// MessageReceived
void
SMLooper::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case MSG_BLOCK:
		{
			bigtime_t now = system_time();
			if (now < fUnblockTime) {
				// port capacity is 1 => the following message blocks the
				// port until return from MessageReceived().
				PostMessage(MSG_UNBLOCK, this);
				snooze_until(fUnblockTime, B_SYSTEM_TIMEBASE);
			}
			break;
		}
		case MSG_UNBLOCK:
			break;
		case MSG_TEST:
			SetDeliveryTime(system_time());
			if (fReplyDelay > 0)
				snooze(fReplyDelay);
			message->SendReply(MSG_REPLY);
			break;
		case MSG_REPLY:
			fReplyTime = system_time();
			break;
		default:
			BLooper::MessageReceived(message);
			break;
	}
}

// BlockUntil
void
SMLooper::BlockUntil(bigtime_t unblockTime)
{
	fUnblockTime = unblockTime;
	PostMessage(MSG_BLOCK, this);
}

// SetReplyDelay
void
SMLooper::SetReplyDelay(bigtime_t replyDelay)
{
	fReplyDelay = replyDelay;
}

// ReplyDelay
bigtime_t
SMLooper::ReplyDelay() const
{
	return fReplyDelay;
}

// DeliverySuccess
bool
SMLooper::DeliverySuccess() const
{
	return (fDeliveryTime >= 0);
}

// SetDeliveryTime
void
SMLooper::SetDeliveryTime(bigtime_t deliveryTime)
{
	fDeliveryTime = deliveryTime;
}

// DeliveryTime
bigtime_t
SMLooper::DeliveryTime() const
{
	return fDeliveryTime;
}

// ReplySuccess
bool
SMLooper::ReplySuccess() const
{
	return (fReplyTime >= 0);
}

// SetReplyTime
void
SMLooper::SetReplyTime(bigtime_t replyTime)
{
	fReplyTime = replyTime;
}

// ReplyTime
bigtime_t
SMLooper::ReplyTime() const
{
	return fReplyTime;
}


// SMHandler

// constructor
SMHandler::SMHandler()
		 : BHandler()
{
}

// MessageReceived
void
SMHandler::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case MSG_TEST:
			if (SMLooper *looper = dynamic_cast<SMLooper*>(Looper())) {
				looper->SetDeliveryTime(system_time());
				if (looper->ReplyDelay() > 0)
					snooze(looper->ReplyDelay());
			}
			message->SendReply(MSG_REPLY);
			break;
		case MSG_REPLY:
			if (SMLooper *looper = dynamic_cast<SMLooper*>(Looper()))
				looper->SetReplyTime(system_time());
			break;
		default:
			BHandler::MessageReceived(message);
			break;
	}
}


