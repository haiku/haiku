/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <map>
#include <new>
#include <set>
#include <string.h>

#include <AutoDeleter.h>
#include <Autolock.h>
#include <DataIO.h>
#include <MessagePrivate.h>
#include <MessengerPrivate.h>
#include <OS.h>
#include <TokenSpace.h>
#include <util/DoublyLinkedList.h>

#include <messaging.h>

#include "Debug.h"
#include "MessageDeliverer.h"
#include "Referenceable.h"

using std::map;
using std::nothrow;
using std::set;

// sDeliverer -- the singleton instance
MessageDeliverer *MessageDeliverer::sDeliverer = NULL;

static const bigtime_t	kRetryDelay			= 100000;			// 100 ms

// per port sanity limits
static const int32		kMaxMessagesPerPort	= 10000;
static const int32		kMaxDataPerPort		= 50 * 1024 * 1024;	// 50 MB


// MessagingTargetSet

// destructor
MessagingTargetSet::~MessagingTargetSet()
{
}


// #pragma mark -

// DefaultMessagingTargetSet

// constructor
DefaultMessagingTargetSet::DefaultMessagingTargetSet(
		const messaging_target *targets, int32 targetCount)
	: MessagingTargetSet(),
	  fTargets(targets),
	  fTargetCount(targetCount),
	  fNextIndex(0)
{
}

// destructor
DefaultMessagingTargetSet::~DefaultMessagingTargetSet()
{
}

// HasNext
bool
DefaultMessagingTargetSet::HasNext() const
{
	return (fNextIndex < fTargetCount);
}

// Next
bool
DefaultMessagingTargetSet::Next(port_id &port, int32 &token)
{
	if (fNextIndex >= fTargetCount)
		return false;

	port = fTargets[fNextIndex].port;
	token = fTargets[fNextIndex].token;
	fNextIndex++;

	return true;
}

// Rewind
void
DefaultMessagingTargetSet::Rewind()
{
	fNextIndex = 0;
}


// #pragma mark -

// SingleMessagingTargetSet

// constructor
SingleMessagingTargetSet::SingleMessagingTargetSet(BMessenger target)
	: MessagingTargetSet(),
	  fAtBeginning(true)
{
	BMessenger::Private messengerPrivate(target);
	fPort = messengerPrivate.Port();
	fToken = (messengerPrivate.IsPreferredTarget()
		? B_PREFERRED_TOKEN : messengerPrivate.Token());
}

// constructor
SingleMessagingTargetSet::SingleMessagingTargetSet(port_id port, int32 token)
	: MessagingTargetSet(),
	  fPort(port),
	  fToken(token),
	  fAtBeginning(true)
{
}

// destructor
SingleMessagingTargetSet::~SingleMessagingTargetSet()
{
}

// HasNext
bool
SingleMessagingTargetSet::HasNext() const
{
	return fAtBeginning;
}

// Next
bool
SingleMessagingTargetSet::Next(port_id &port, int32 &token)
{
	if (!fAtBeginning)
		return false;

	port = fPort;
	token = fToken;
	fAtBeginning = false;

	return true;
}

// Rewind
void
SingleMessagingTargetSet::Rewind()
{
	fAtBeginning = true;
}


// #pragma mark -

// Message
/*!	\brief Encapsulates a message to be delivered.

	Besides the flattened message it also stores the when the message was
	created and when the delivery attempts shall time out.
*/
class MessageDeliverer::Message : public BReferenceable {
public:
	Message(void *data, int32 dataSize, bigtime_t timeout)
		: BReferenceable(),
		  fData(data),
		  fDataSize(dataSize),
		  fCreationTime(system_time()),
		  fBusy(false)
	{
		if (B_INFINITE_TIMEOUT - fCreationTime <= timeout)
			fTimeoutTime = B_INFINITE_TIMEOUT;
		else if (timeout <= 0)
			fTimeoutTime = fCreationTime;
		else
			fTimeoutTime = fCreationTime + timeout;
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

	bigtime_t CreationTime() const
	{
		return fCreationTime;
	}

	bigtime_t TimeoutTime() const
	{
		return fTimeoutTime;
	}

	bool HasTimeout() const
	{
		return (fTimeoutTime < B_INFINITE_TIMEOUT);
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
	bigtime_t	fTimeoutTime;
	bool		fBusy;
};

// TargetMessage
/*!	\brief Encapsulates a Message to be sent to a specific handler.

	A TargetMessage is always associated with (i.e. queued in) a TargetPort.
	While a Message stores only the message data and some timing info, this
	object adds the token of a the target BHandler.

	A Message can be referred to by more than one TargetMessage (when
	broadcasting), but a TargetMessage is referred to exactly once, by
	the TargetPort.
*/
class MessageDeliverer::TargetMessage
	: public DoublyLinkedListLinkImpl<MessageDeliverer::TargetMessage> {
public:
	TargetMessage(Message *message, int32 token)
		: fMessage(message),
		  fToken(token)
	{
		if (fMessage)
			fMessage->AcquireReference();
	}

	~TargetMessage()
	{
		if (fMessage)
			fMessage->ReleaseReference();
	}

	Message *GetMessage() const
	{
		return fMessage;
	}

	int32 Token() const
	{
		return fToken;
	}

private:
	Message				*fMessage;
	int32				fToken;
};

// TargetMessageHandle
/*!	\brief A small wrapper for TargetMessage providing a complete order.

	This class only exists to provide the comparison operators required to
	put a TargetMessage into a set. The order implemented is by ascending by
	timeout time (primary) and by TargetMessage pointer (secondary).
	Hence TargetMessageHandles referring to the same TargetMessage are equal
	(and only those).
*/
class MessageDeliverer::TargetMessageHandle {
public:
	TargetMessageHandle(TargetMessage *message)
		: fMessage(message)
	{
	}

	TargetMessageHandle(const TargetMessageHandle &other)
		: fMessage(other.fMessage)
	{
	}

	TargetMessage *GetMessage() const
	{
		return fMessage;
	}

	TargetMessageHandle &operator=(const TargetMessageHandle &other)
	{
		fMessage = other.fMessage;
		return *this;
	}

	bool operator==(const TargetMessageHandle &other) const
	{
		return (fMessage == other.fMessage);
	}

	bool operator!=(const TargetMessageHandle &other) const
	{
		return (fMessage != other.fMessage);
	}

	bool operator<(const TargetMessageHandle &other) const
	{
		bigtime_t timeout = fMessage->GetMessage()->TimeoutTime();
		bigtime_t otherTimeout = other.fMessage->GetMessage()->TimeoutTime();
		if (timeout < otherTimeout)
			return true;
		if (timeout > otherTimeout)
			return false;
		return (fMessage < other.fMessage);
	}

private:
	TargetMessage	*fMessage;
};

// TargetPort
/*!	\brief Represents a full target port, queuing the not yet delivered
		   messages.

	A TargetPort internally queues TargetMessages in the order the are to be
	delivered. Furthermore the object maintains an ordered set of
	TargetMessages that can timeout (in ascending order of timeout time), so
	that timed out messages can be dropped easily.
*/
class MessageDeliverer::TargetPort {
public:
	TargetPort(port_id portID)
		: fPortID(portID),
		  fMessages(),
		  fMessageCount(0),
		  fMessageSize(0)
	{
	}

	~TargetPort()
	{
		while (!fMessages.IsEmpty())
			PopMessage();
	}

	port_id PortID() const
	{
		return fPortID;
	}

	status_t PushMessage(Message *message, int32 token)
	{
PRINT("MessageDeliverer::TargetPort::PushMessage(port: %ld, %p, %ld)\n",
fPortID, message, token);
		// create a target message
		TargetMessage *targetMessage
			= new(nothrow) TargetMessage(message, token);
		if (!targetMessage)
			return B_NO_MEMORY;

		// push it
		fMessages.Insert(targetMessage);
		fMessageCount++;
		fMessageSize += targetMessage->GetMessage()->DataSize();

		// add it to the timeoutable messages, if it has a timeout
		if (message->HasTimeout())
			fTimeoutableMessages.insert(targetMessage);

		_EnforceLimits();

		return B_OK;
	}

	Message *PeekMessage(int32 &token) const
	{
		if (!fMessages.Head())
			return NULL;

		token = fMessages.Head()->Token();
		return fMessages.Head()->GetMessage();
	}

	void PopMessage()
	{
		if (fMessages.Head()) {
PRINT("MessageDeliverer::TargetPort::PopMessage(): port: %ld, %p\n",
fPortID, fMessages.Head()->GetMessage());
			_RemoveMessage(fMessages.Head());
		}
	}

	void DropTimedOutMessages()
	{
		bigtime_t now = system_time();

		while (fTimeoutableMessages.begin() != fTimeoutableMessages.end()) {
			TargetMessage *message = fTimeoutableMessages.begin()->GetMessage();
			if (message->GetMessage()->TimeoutTime() > now)
				break;

PRINT("MessageDeliverer::TargetPort::DropTimedOutMessages(): port: %ld: "
"message %p timed out\n", fPortID, message->GetMessage());
			_RemoveMessage(message);
		}
	}

	bool IsEmpty() const
	{
		return fMessages.IsEmpty();
	}

private:
	void _RemoveMessage(TargetMessage *message)
	{
		fMessages.Remove(message);
		fMessageCount--;
		fMessageSize -= message->GetMessage()->DataSize();

		if (message->GetMessage()->HasTimeout())
			fTimeoutableMessages.erase(message);

		delete message;
	}

	void _EnforceLimits()
	{
		// message count
		while (fMessageCount > kMaxMessagesPerPort) {
PRINT("MessageDeliverer::TargetPort::_EnforceLimits(): port: %ld: hit maximum "
"message count limit.\n", fPortID);
			PopMessage();
		}

		// message size
		while (fMessageSize > kMaxDataPerPort) {
PRINT("MessageDeliverer::TargetPort::_EnforceLimits(): port: %ld: hit maximum "
"message size limit.\n", fPortID);
			PopMessage();
		}
	}

	typedef DoublyLinkedList<TargetMessage>	MessageList;

	port_id						fPortID;
	MessageList					fMessages;
	int32						fMessageCount;
	int32						fMessageSize;
	set<TargetMessageHandle>	fTimeoutableMessages;
};

// TargetPortMap
struct MessageDeliverer::TargetPortMap : public map<port_id, TargetPort*> {
};


// #pragma mark -

/*!	\class MessageDeliverer
	\brief Service for delivering messages, which retries the delivery as long
		   as the target port is full.

	For the user of the service only the MessageDeliverer::DeliverMessage()
	will be of interest. Some of them allow broadcasting a message to several
	recepients.

	The class maintains a TargetPort for each target port which was full at the
	time a message was to be delivered to it. A TargetPort has a queue of
	undelivered messages. A separate worker thread retries periodically to send
	the yet undelivered messages to the respective target ports.
*/

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
		"message deliverer", B_NORMAL_PRIORITY + 1, this);
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
/*!	\brief Delivers a message to the supplied target.

	The method tries to send the message right now (if there are not already
	messages pending for the target port). If that fails due to a full target
	port, the message is queued for later delivery.

	\param message The message to be delivered.
	\param target A BMessenger identifying the delivery target.
	\param timeout If given, the message will be dropped, when it couldn't be
		   delivered after this amount of microseconds.
	\return
	- \c B_OK, if sending the message succeeded or if the target port was
	  full and the message has been queued,
	- another error code otherwise.
*/
status_t
MessageDeliverer::DeliverMessage(BMessage *message, BMessenger target,
	bigtime_t timeout)
{
	SingleMessagingTargetSet set(target);
	return DeliverMessage(message, set, timeout);
}

// DeliverMessage
/*!	\brief Delivers a message to the supplied targets.

	The method tries to send the message right now to each of the given targets
	(if there are not already messages pending for a target port). If that
	fails due to a full target port, the message is queued for later delivery.

	\param message The message to be delivered.
	\param targets MessagingTargetSet providing the the delivery targets.
	\param timeout If given, the message will be dropped, when it couldn't be
		   delivered after this amount of microseconds.
	\return
	- \c B_OK, if for each of the given targets sending the message succeeded
	  or if the target port was full and the message has been queued,
	- another error code otherwise.
*/
status_t
MessageDeliverer::DeliverMessage(BMessage *message, MessagingTargetSet &targets,
	bigtime_t timeout)
{
	if (!message)
		return B_BAD_VALUE;

	// flatten the message
	BMallocIO mallocIO;
	status_t error = message->Flatten(&mallocIO, NULL);
	if (error < B_OK)
		return error;

	return DeliverMessage(mallocIO.Buffer(), mallocIO.BufferLength(), targets,
		timeout);
}

// DeliverMessage
/*!	\brief Delivers a flattened message to the supplied targets.

	The method tries to send the message right now to each of the given targets
	(if there are not already messages pending for a target port). If that
	fails due to a full target port, the message is queued for later delivery.

	\param message The flattened message to be delivered. This may be a
		   flattened BMessage or KMessage.
	\param messageSize The size of the flattened message buffer.
	\param targets MessagingTargetSet providing the the delivery targets.
	\param timeout If given, the message will be dropped, when it couldn't be
		   delivered after this amount of microseconds.
	\return
	- \c B_OK, if for each of the given targets sending the message succeeded
	  or if the target port was full and the message has been queued,
	- another error code otherwise.
*/
status_t
MessageDeliverer::DeliverMessage(const void *messageData, int32 messageSize,
	MessagingTargetSet &targets, bigtime_t timeout)
{
	if (!messageData || messageSize <= 0)
		return B_BAD_VALUE;

	// clone the buffer
	void *data = malloc(messageSize);
	if (!data)
		return B_NO_MEMORY;
	memcpy(data, messageData, messageSize);

	// create a Message
	Message *message = new(nothrow) Message(data, messageSize, timeout);
	if (!message) {
		free(data);
		return B_NO_MEMORY;
	}
	BReference<Message> _(message, true);

	// add the message to the respective target ports
	BAutolock locker(fLock);
	for (int32 targetIndex = 0; targets.HasNext(); targetIndex++) {
		port_id portID;
		int32 token;
		targets.Next(portID, token);

		// get the target port
		TargetPort *port = _GetTargetPort(portID, true);
		if (!port)
			return B_NO_MEMORY;

		// try sending the message, if there are no queued messages yet
		if (port->IsEmpty()) {
			status_t error = _SendMessage(message, portID, token);
			// if the message was delivered OK, we're done with the target
			if (error == B_OK) {
				_PutTargetPort(port);
				continue;
			}

			// if the port is not full, but an error occurred, we skip this target
			if (error != B_WOULD_BLOCK) {
				_PutTargetPort(port);
				if (targetIndex == 0 && !targets.HasNext())
					return error;
				continue;
			}
		}

		// add the message
		status_t error = port->PushMessage(message, token);
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

	if (port->IsEmpty()) {
		fTargetPorts->erase(port->PortID());
		delete port;
	}
}

// _SendMessage
status_t
MessageDeliverer::_SendMessage(Message *message, port_id portID, int32 token)
{
	status_t error = BMessage::Private::SendFlattenedMessage(message->Data(),
		message->DataSize(), portID, token, 0);
//PRINT("MessageDeliverer::_SendMessage(%p, port: %ld, token: %ld): %lx\n",
//message, portID, token, error);
	return error;
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
	while (!fTerminating) {
		snooze(kRetryDelay);
		if (fTerminating)
			break;

		// iterate through all target ports and try sending the messages
		BAutolock _(fLock);
		for (TargetPortMap::iterator it = fTargetPorts->begin();
			 it != fTargetPorts->end();) {
			TargetPort *port = it->second;
			bool portError = false;

			port->DropTimedOutMessages();

			// try sending all messages
			int32 token;
			while (Message *message = port->PeekMessage(token)) {
				status_t error = B_OK;
//				if (message->TimeoutTime() > system_time()) {
					error = _SendMessage(message, port->PortID(), token);
//				} else {
//					// timeout, drop message
//					PRINT("MessageDeliverer::_DelivererThread(): port %ld, "
//						"message %p timed out\n", port->PortID(), message);
//				}

				if (error == B_OK) {
					port->PopMessage();
				} else if (error == B_WOULD_BLOCK) {
					// no luck yet -- port is still full
					break;
				} else {
					// unexpected error -- probably the port is gone
					portError = true;
					break;
				}
			}

			// next port
			if (portError || port->IsEmpty()) {
				TargetPortMap::iterator oldIt = it;
				++it;
				delete port;
				fTargetPorts->erase(oldIt);
			} else
				++it;
		}
	}

	return 0;
}
