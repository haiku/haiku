// SMLooper.h

#ifndef SM_LOOPER_H
#define SM_LOOPER_H

#include <Handler.h>
#include <Looper.h>

class SMLooper : public BLooper {
public:
	SMLooper();
	virtual ~SMLooper();

	virtual void MessageReceived(BMessage *message);

	void BlockUntil(bigtime_t unblockTime);

	void SetReplyDelay(bigtime_t replyDelay);
	bigtime_t ReplyDelay() const;

	bool DeliverySuccess() const;
	void SetDeliveryTime(bigtime_t deliveryTime);
	bigtime_t DeliveryTime() const;

	bool ReplySuccess() const;
	void SetReplyTime(bigtime_t replyTime);
	bigtime_t ReplyTime() const;

private:
	bigtime_t	fUnblockTime;
	bigtime_t	fReplyDelay;
	bigtime_t	fDeliveryTime;
	bigtime_t	fReplyTime;
};

class SMHandler : public BHandler {
public:
	SMHandler();

	virtual void MessageReceived(BMessage *message);
};

#endif	// SM_LOOPER_H
