/* 
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <map>
#include <new>

#include <Autolock.h>
#include <DataIO.h>
#include <MessengerPrivate.h>
#include <OS.h>
#include <TokenSpace.h>

#include <messaging.h>

#include "MessageDeliverer.h"
#include "Referencable.h"

// sDeliverer -- the singleton instance
MessageDeliverer *MessageDeliverer::sDeliverer = NULL;

// Message
class MessageDeliverer::Message : public Referencable {
public:
	Message(void *data, int32 dataSize)
		: Referencable(true),
		  fData(data),
		  fDataSize(dataSize),
		  fCreationTime(system_time()),
		  fBusy(false)
	{
	}

	~Message()
	{
		free(fData);
	}

	void *Data() const
	{
		return fData;
	}

	int32 DataSize() const
	{
		return fDataSize;
	}

	bigtime_t CreationTime()
	{
		return fCreationTime;
	}

	void SetBusy(bool busy)
	{
		fBusy = busy;
	}

	bool IsBusy() const
	{
		return fBusy;
	}

private:
	void		*fData;
	int32		fDataSize;
	bigtime_t	fCreationTime;
	bool		fBusy;
};

// TargetMessage
class MessageDeliverer::TargetMessage {
public:
	TargetMessage(Message *message, int32 token)
		: fMessage(message),
		  fToken(token),
		  fNext(NULL)
	{
		if (fMessage)
			fMessage->AddReference();
	}

	~TargetMessage()
	{
		if (fMessage)
			fMessage->RemoveReference();
	}

	Message *GetMessage() const
	{
		return fMessage;
	}

	int32 Token() const
	{
		return fToken;
	}

	void SetNext(TargetMessage *next)
	{
		fNext = next;
	}

	TargetMessage *Next() const
	{
		return fNext;
	}

private:
	Message			*fMessage;
	int32			fToken;
	TargetMessage	*fNext;
};

// TargetPort
class MessageDeliverer::TargetPort {
public:
	TargetPort(port_id portID)
		: fPortID(portID),
		  fFirstMessage(NULL),
		  fLastMessage(NULL)
	{
	}

	port_id PortID() const
	{
		return fPortID;
	}

	status_t PushMessage(Message *message, int32 token)
	{
		// create a target message
		TargetMessage *targetMessage
			= new(nothrow) TargetMessage(message, token);
		if (!targetMessage)
			return B_NO_MEMORY;

		// push it
		if (fLastMessage)
			fLastMessage->SetNext(targetMessage);
		else
			fFirstMessage = fLastMessage = targetMessage;

		return B_OK;
	}

	Message *PeekMessage(int32 &token)
	{
		if (!fFirstMessage)
			return NULL;

		token = fFirstMessage->Token();
		return fFirstMessage->GetMessage();
	}

	void PopMessage()
	{
		if (fFirstMessage) {
			TargetMessage *message = fFirstMessage;
			fFirstMessage = message->Next();
			if (!fFirstMessage)
				fLastMessage = NULL;
			delete message;
		}
	}

private:
	port_id			fPortID;
	TargetMessage	*fFirstMessage;
	TargetMessage	*fLastMessage;
};

// TargetPortMap
struct MessageDeliverer::TargetPortMap : public map<port_id, TargetPort*> {
};

// constructor
MessageDeliverer::MessageDeliverer()
	: fLock("message deliverer"),
	  fTargetPorts(NULL),
	  fDelivererThread(-1),
	  fTerminating(false)
{
}

// destructor
MessageDeliverer::~MessageDeliverer()
{
	fTerminating = true;

// TODO: How to stop the thread?

	if (fDelivererThread >= 0) {
		int32 result;
		wait_for_thread(fDelivererThread, &result);
	}

	delete fTargetPorts;
}

// Init
status_t
MessageDeliverer::Init()
{
	// create the target port map
	fTargetPorts = new(nothrow) TargetPortMap;
	if (!fTargetPorts)
		return B_NO_MEMORY;

	// spawn the deliverer thread
	fDelivererThread = spawn_thread(MessageDeliverer::_DelivererThreadEntry,
		"message deliverer", B_NORMAL_PRIORITY, this);
	if (fDelivererThread < 0)
		return fDelivererThread;

	// resume the deliverer thread
	resume_thread(fDelivererThread);

	return B_OK;
}

// CreateDefault
status_t
MessageDeliverer::CreateDefault()
{
	if (sDeliverer)
		return B_OK;

	// create the deliverer
	MessageDeliverer *deliverer = new(nothrow) MessageDeliverer;
	if (!deliverer)
		return B_NO_MEMORY;

	// init it
	status_t error = deliverer->Init();
	if (error != B_OK) {
		delete deliverer;
		return error;
	}

	sDeliverer = deliverer;
	return B_OK;
}

// DeleteDefault
void
MessageDeliverer::DeleteDefault()
{
	if (sDeliverer) {
		delete sDeliverer;
		sDeliverer = NULL;
	}
}

// Default
MessageDeliverer *
MessageDeliverer::Default()
{
	return sDeliverer;
}

// DeliverMessage
status_t
MessageDeliverer::DeliverMessage(BMessage *message, BMessenger target)
{
	BMessenger::Private messengerPrivate(target);
	return DeliverMessage(message, messengerPrivate.Port(),
		messengerPrivate.IsPreferredTarget()
			? B_PREFERRED_TOKEN : messengerPrivate.Token());
}

// DeliverMessage
status_t
MessageDeliverer::DeliverMessage(BMessage *message, port_id port, int32 token)
{
	if (!message)
		return B_BAD_VALUE;

	// flatten the message
	BMallocIO mallocIO;
	status_t error = message->Flatten(&mallocIO);
	if (error != B_OK)
		return error;

	return DeliverMessage(mallocIO.Buffer(), mallocIO.BufferLength(), port,
		token);
}

// DeliverMessage
status_t
MessageDeliverer::DeliverMessage(const void *message, int32 messageSize,
	BMessenger target)
{
	BMessenger::Private messengerPrivate(target);
	return DeliverMessage(message, messageSize, messengerPrivate.Port(),
		messengerPrivate.IsPreferredTarget()
			? B_PREFERRED_TOKEN : messengerPrivate.Token());
}

// DeliverMessage
status_t
MessageDeliverer::DeliverMessage(const void *message, int32 messageSize,
	port_id port, int32 token)
{
	messaging_target target;
	target.port = port;
	target.token = token;
	return DeliverMessage(message, messageSize, &target, 1);
}

// DeliverMessage
status_t
MessageDeliverer::DeliverMessage(const void *messageData, int32 messageSize,
	const messaging_target *targets, int32 targetCount)
{
	if (!messageData || messageSize <= 0)
		return B_BAD_VALUE;

	// clone the buffer
	void *data = malloc(messageSize);
	if (!data)
		return B_NO_MEMORY;
	memcpy(data, messageData, messageSize);

	// create a Message
	Message *message = new(nothrow) Message(data, messageSize);
	if (!message) {
		free(data);
		return B_NO_MEMORY;
	}
	Reference<Message> _(message, true);

	// add the message to the respective target ports
	BAutolock locker(fLock);
	for (int32 i = 0; i < targetCount; i++) {
		// get the target port
		TargetPort *port = _GetTargetPort(targets[i].port, true);
		if (!port)
			return B_NO_MEMORY;

		// add the message
		status_t error = port->PushMessage(message, targets[i].token);
		_PutTargetPort(port);
		if (error != B_OK)
			return error;
	}

	return B_OK;
}

// _GetTargetPort
MessageDeliverer::TargetPort *
MessageDeliverer::_GetTargetPort(port_id portID, bool create)
{
	// get the port from the map
	TargetPortMap::iterator it = fTargetPorts->find(portID);
	if (it != fTargetPorts->end())
		return it->second;

	if (!create)
		return NULL;
	
	// create a port
	TargetPort *port = new(nothrow) TargetPort(portID);
	if (!port)
		return NULL;
	(*fTargetPorts)[portID] = port;

	return port;
}

// _PutTargetPort
void
MessageDeliverer::_PutTargetPort(TargetPort *port)
{
	if (!port)
		return;

	int32 token;
	if (!port->PeekMessage(token)) {
		fTargetPorts->erase(port->PortID());
		delete port;
	}
}

// _DelivererThreadEntry
int32
MessageDeliverer::_DelivererThreadEntry(void *data)
{
	return ((MessageDeliverer*)data)->_DelivererThread();
}

// _DelivererThread
int32
MessageDeliverer::_DelivererThread()
{
//	while (fTerminating) {
//	}

	return 0;
}
