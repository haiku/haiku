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

// RemoteSMTarget
class RemoteSMTarget : public SMTarget {
public:
	RemoteSMTarget(bool preferred);
	virtual ~RemoteSMTarget();

	virtual void Init(bigtime_t unblockTime, bigtime_t replyDelay);

	virtual BMessenger Messenger();

	virtual bool DeliverySuccess();

private:
	status_t _SendRequest(int32 code, const void *buffer = NULL,
						  size_t size = 0);
	status_t _GetReply(int32 code, void *buffer, size_t size);

private:
	port_id		fLocalPort;
	port_id		fRemotePort;
	BMessenger	fTarget;

	static int32	fID;
};

#endif	// SM_TARGET_H
