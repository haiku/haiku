// SMTarget.h

#ifndef SM_TARGET_H
#define SM_TARGET_H

#include <Messenger.h>

class SMHandler;
class SMLooper;

// SMTarget
class SMTarget {
public:
	SMTarget();
	virtual ~SMTarget();

	virtual void Init(bigtime_t unblockTime, bigtime_t replyDelay);

	virtual BHandler *Handler();
	virtual BMessenger Messenger();
	virtual bool DeliverySuccess();
};

// LocalSMTarget
class LocalSMTarget : public SMTarget {
public:
	LocalSMTarget(bool preferred);
	virtual ~LocalSMTarget();

	virtual void Init(bigtime_t unblockTime, bigtime_t replyDelay);

	virtual BHandler *Handler();
	virtual BMessenger Messenger();

	virtual bool DeliverySuccess();

private:
	SMHandler	*fHandler;
	SMLooper	*fLooper;
};

#endif	// SM_TARGET_H
