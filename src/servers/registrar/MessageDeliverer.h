/* 
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef MESSAGE_DELIVERER_H
#define MESSAGE_DELIVERER_H

#include <Locker.h>
#include <Messenger.h>

struct messaging_target;

// MessagingTargetSet
class MessagingTargetSet {
public:
	virtual ~MessagingTargetSet();

	virtual bool HasNext() const = 0;
	virtual bool Next(port_id &port, int32 &token) = 0;
	virtual void Rewind() = 0;
};

// DefaultMessagingTargetSet
class DefaultMessagingTargetSet : public MessagingTargetSet {
public:
	DefaultMessagingTargetSet(const messaging_target *targets,
		int32 targetCount);
	virtual ~DefaultMessagingTargetSet();

	virtual bool HasNext() const;
	virtual bool Next(port_id &port, int32 &token);
	virtual void Rewind();

private:
	const messaging_target	*fTargets;
	int32					fTargetCount;
	int32					fNextIndex;
};

// SingleMessagingTargetSet
class SingleMessagingTargetSet : public MessagingTargetSet {
public:
	SingleMessagingTargetSet(BMessenger target);
	SingleMessagingTargetSet(port_id port, int32 token);
	virtual ~SingleMessagingTargetSet();

	virtual bool HasNext() const;
	virtual bool Next(port_id &port, int32 &token);
	virtual void Rewind();

private:
	port_id				fPort;
	int32				fToken;
	bool				fAtBeginning;
};

// MessageDeliverer
class MessageDeliverer {
private:
	MessageDeliverer();
	~MessageDeliverer();

	status_t Init();

public:
	static status_t CreateDefault();
	static void DeleteDefault();
	static MessageDeliverer *Default();

	status_t DeliverMessage(BMessage *message, BMessenger target,
		bigtime_t timeout = B_INFINITE_TIMEOUT);
	status_t DeliverMessage(BMessage *message, MessagingTargetSet &targets,
		bigtime_t timeout = B_INFINITE_TIMEOUT);
	status_t DeliverMessage(const void *message, int32 messageSize,
		MessagingTargetSet &targets, bigtime_t timeout = B_INFINITE_TIMEOUT);

private:
	class Message;
	class TargetMessage;
	class TargetMessageHandle;
	class TargetPort;
	struct TargetPortMap;

	TargetPort *_GetTargetPort(port_id portID, bool create = false);
	void _PutTargetPort(TargetPort *port);

	status_t _SendMessage(Message *message, port_id portID, int32 token);

	static int32 _DelivererThreadEntry(void *data);
	int32 _DelivererThread();

	static MessageDeliverer	*sDeliverer;

	BLocker			fLock;
	TargetPortMap	*fTargetPorts;
	thread_id		fDelivererThread;
	volatile bool	fTerminating;
};

#endif	// MESSAGE_DELIVERER_H
