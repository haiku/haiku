/* 
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef MESSAGE_DELIVERER_H
#define MESSAGE_DELIVERER_H

#include <Locker.h>
#include <Messenger.h>

struct messaging_target;

class MessageDeliverer {
private:
	MessageDeliverer();
	~MessageDeliverer();

	status_t Init();

public:
	static status_t CreateDefault();
	static void DeleteDefault();
	static MessageDeliverer *Default();

	status_t DeliverMessage(BMessage *message, BMessenger target);
	status_t DeliverMessage(BMessage *message, port_id port, int32 token);
	status_t DeliverMessage(const void *message, int32 messageSize,
		BMessenger target);
	status_t DeliverMessage(const void *message, int32 messageSize,
		port_id port, int32 token);
	status_t DeliverMessage(const void *message, int32 messageSize,
		const messaging_target *targets, int32 targetCount);

private:
	class Message;
	class TargetMessage;
	class TargetPort;
	struct TargetPortMap;

	TargetPort *_GetTargetPort(port_id portID, bool create = false);
	void _PutTargetPort(TargetPort *port);

	static int32 _DelivererThreadEntry(void *data);
	int32 _DelivererThread();

	static MessageDeliverer	*sDeliverer;

	BLocker			fLock;
	TargetPortMap	*fTargetPorts;
	thread_id		fDelivererThread;
	volatile bool	fTerminating;
};

#endif	// MESSAGE_DELIVERER_H
