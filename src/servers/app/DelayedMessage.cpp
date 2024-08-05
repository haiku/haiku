/*
 * Copyright 2015, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *			Joseph Groover <looncraz@looncraz.net>
*/


#include "DelayedMessage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Autolock.h>
#include <String.h>

#include <LinkSender.h>
#include <ServerProtocol.h>


// DelayedMessageSender constants
static const int32 kWakeupMessage = AS_LAST_CODE + 2048;
static const int32 kExitMessage = kWakeupMessage + 1;

static const char* kName = "DMT is here for you, eventually...";
static int32 kPriority = B_URGENT_DISPLAY_PRIORITY;
static int32 kPortCapacity = 10;


//! Data attachment structure.
struct Attachment {
								Attachment(const void* data, size_t size);
								~Attachment();

			const void*			constData;
			void*				data;
			size_t				size;
};


typedef BObjectList<Attachment> AttachmentList;


/*!	\class ScheduledMessage
	\brief Responsible for sending of delayed message.
*/
class ScheduledMessage {
public:
								ScheduledMessage(DelayedMessage& message);
								~ScheduledMessage();

			int32				CountTargets() const;

			void				Finalize();
			bigtime_t			ScheduledTime() const;
			int32				SendMessage();
			bool				IsValid() const;
			bool				Merge(DelayedMessage& message);

			status_t			SendMessageToPort(port_id port);
			bool 				operator<(const ScheduledMessage& other) const;

			DelayedMessageData*	fData;
};


/*!	\class DelayedMessageSender DelayedMessageSender.h
	\brief Responsible for scheduling and sending of delayed messages
*/
class DelayedMessageSender {
public:
			explicit			DelayedMessageSender();
								~DelayedMessageSender();

			status_t			ScheduleMessage	(DelayedMessage& message);

			int32				CountDelayedMessages() const;
			int64				CountSentMessages() const;

private:
			void				_MessageLoop();
			int32				_SendDelayedMessages();
	static	int32				_thread_func(void* sender);
			void				_Wakeup(bigtime_t whatTime);

private:
	typedef BObjectList<ScheduledMessage> ScheduledList;

	mutable	BLocker				fLock;
			ScheduledList		fMessages;

			bigtime_t			fScheduledWakeup;

			int32				fWakeupRetry;
			thread_id			fThread;
			port_id				fPort;

	mutable	int64				fSentCount;
};


DelayedMessageSender gDelayedMessageSender;


/*!	\class DelayedMessageData DelayedMessageSender.h
	\brief Owns DelayedMessage data, allocates memory and copies data only
			when needed,
*/
class DelayedMessageData {
	typedef BObjectList<port_id> PortList;
	typedef void(*FailureCallback)(int32 code, port_id port, void* data);
public:
								DelayedMessageData(int32 code, bigtime_t delay,
									bool isSpecificTime);
								~DelayedMessageData();

			bool				AddTarget(port_id port);
			void				RemoveTarget(port_id port);
			int32				CountTargets() const;

			void				MergeTargets(DelayedMessageData* other);

			bool				CopyData();
			bool				MergeData(DelayedMessageData* other);

			bool				IsValid() const;
				// Only valid after a successful CopyData().

			status_t			Attach(const void* data, size_t size);

			bool				Compare(Attachment* one, Attachment* two,
									int32 index);

			void				SetMerge(DMMergeMode mode, uint32 mask);
			void				SendFailed(port_id port);

			void				SetFailureCallback(FailureCallback callback,
									void* data);

			// Accessors.
			int32&				Code() {return fCode;}
			const int32&		Code() const {return fCode;}

			bigtime_t&			ScheduledTime() {return fScheduledTime;}
			const bigtime_t&	ScheduledTime() const {return fScheduledTime;}

			AttachmentList&		Attachments() {return fAttachments;}
			const AttachmentList&	Attachments() const {return fAttachments;}

			PortList&			Targets() {return fTargets;}
			const PortList&		Targets() const {return fTargets;}

private:
		// Data members.

			int32				fCode;
			bigtime_t			fScheduledTime;
			bool				fValid;

			AttachmentList		fAttachments;
			PortList			fTargets;

			DMMergeMode			fMergeMode;
			uint32				fMergeMask;

			FailureCallback		fFailureCallback;
			void*				fFailureData;
};


// #pragma mark -



DelayedMessage::DelayedMessage(int32 code, bigtime_t delay,
		bool isSpecificTime)
	:
	fData(new(std::nothrow) DelayedMessageData(code, delay < DM_MINIMUM_DELAY
		? DM_MINIMUM_DELAY : delay, isSpecificTime)),
	fHandedOff(false)
{
}


DelayedMessage::~DelayedMessage()
{
	// Message is canceled without a handoff.
	if (!fHandedOff)
		delete fData;
}


bool
DelayedMessage::AddTarget(port_id port)
{
	if (fData == NULL || fHandedOff)
		return false;

	return fData->AddTarget(port);
}


void
DelayedMessage::SetMerge(DMMergeMode mode, uint32 match)
{
	if (fData == NULL || fHandedOff)
		return;

	fData->SetMerge(mode, match);
}


void
DelayedMessage::SetFailureCallback(void (*callback)(int32, port_id, void*),
	void* data)
{
	if (fData == NULL || fHandedOff)
		return;

	fData->SetFailureCallback(callback, data);
}


//! Attach data to message. Memory is not allocated nor copied until handoff.
status_t
DelayedMessage::Attach(const void* data, size_t size)
{
	if (fData == NULL)
		return B_NO_MEMORY;

	if (fHandedOff)
		return B_ERROR;

	if (data == NULL || size == 0)
		return B_BAD_VALUE;

	return	fData->Attach(data, size);
}


status_t
DelayedMessage::Flush()
{
	if (fData == NULL)
		return B_NO_MEMORY;

	if (fHandedOff)
		return B_ERROR;

	if (fData->CountTargets() == 0)
		return B_BAD_VALUE;

	return gDelayedMessageSender.ScheduleMessage(*this);
}


/*!	The data handoff occurs upon scheduling and reduces copies to only
	when a message is actually scheduled. Canceled messages have low cost.
*/
DelayedMessageData*
DelayedMessage::HandOff()
{
	if (fData == NULL || fHandedOff)
		return NULL;

	if (fData->CopyData()) {
		fHandedOff = true;
		return fData;
	}

	return NULL;
}


// #pragma mark -


Attachment::Attachment(const void* _data, size_t _size)
	:
	constData(_data),
	data(NULL),
	size(_size)
{
}


Attachment::~Attachment()
{
	free(data);
}


// #pragma mark -


DelayedMessageData::DelayedMessageData(int32 code, bigtime_t delay,
	bool isSpecificTime)
	:
	fCode(code),
	fScheduledTime(delay + (isSpecificTime ? 0 : system_time())),
	fValid(false),

	fAttachments(3, true),
	fTargets(4, true),

	fMergeMode(DM_NO_MERGE),
	fMergeMask(DM_DATA_DEFAULT),

	fFailureCallback(NULL),
	fFailureData(NULL)
{
}


DelayedMessageData::~DelayedMessageData()
{
}


bool
DelayedMessageData::AddTarget(port_id port)
{
	if (port <= 0)
		return false;

	// check for duplicates:
	for (int32 index = 0; index < fTargets.CountItems(); ++index) {
		if (port == *fTargets.ItemAt(index))
			return false;
	}

	return fTargets.AddItem(new(std::nothrow) port_id(port));
}


void
DelayedMessageData::RemoveTarget(port_id port)
{
	if (port == B_BAD_PORT_ID)
		return;

	// Search for a match by value.
	for (int32 index = 0; index < fTargets.CountItems(); ++index) {
		port_id* target = fTargets.ItemAt(index);
		if (port == *target) {
			fTargets.RemoveItem(target, true);
			return;
		}
	}
}


int32
DelayedMessageData::CountTargets() const
{
	return fTargets.CountItems();
}


void
DelayedMessageData::MergeTargets(DelayedMessageData* other)
{
	// Failure to add one target does not abort the loop!
	// It could just mean we already have the target.
	for (int32 index = 0; index < other->fTargets.CountItems(); ++index)
		AddTarget(*(other->fTargets.ItemAt(index)));
}


//! Copy data from original location - merging failed
bool
DelayedMessageData::CopyData()
{
	Attachment* attached = NULL;

	for (int32 index = 0; index < fAttachments.CountItems(); ++index) {
		attached = fAttachments.ItemAt(index);

		if (attached == NULL || attached->data != NULL)
			return false;

		attached->data = malloc(attached->size);
		if (attached->data == NULL)
			return false;

		memcpy(attached->data, attached->constData, attached->size);
	}

	fValid = true;
	return true;
}


bool
DelayedMessageData::MergeData(DelayedMessageData* other)
{
	if (!fValid
		|| other == NULL
		|| other->fCode != fCode
		|| fMergeMode == DM_NO_MERGE
		|| other->fMergeMode == DM_NO_MERGE
		|| other->fMergeMode != fMergeMode
		|| other->fAttachments.CountItems() != fAttachments.CountItems())
		return false;

	if (other->fMergeMode == DM_MERGE_CANCEL) {
		MergeTargets(other);
		return true;
	}

	// Compare data
	Attachment* attached = NULL;
	Attachment* otherAttached = NULL;

	for (int32 index = 0; index < fAttachments.CountItems(); ++index) {
		attached = fAttachments.ItemAt(index);
		otherAttached = other->fAttachments.ItemAt(index);

		if (attached == NULL
			|| otherAttached == NULL
			|| attached->data == NULL
			|| otherAttached->constData == NULL
			|| attached->size != otherAttached->size)
			return false;

		// Compares depending upon mode & flags
		if (!Compare(attached, otherAttached, index))
			return false;
	}

	// add any targets not included in the existing message!
	MergeTargets(other);

	// since these are duplicates, we need not copy anything...
	if (fMergeMode == DM_MERGE_DUPLICATES)
		return true;

	// DM_MERGE_REPLACE:

	// Import the new data!
	for (int32 index = 0; index < fAttachments.CountItems(); ++index) {
		attached = fAttachments.ItemAt(index);
		otherAttached = other->fAttachments.ItemAt(index);

		// We already have allocated our memory, but the other data
		// has not.  So this reduces memory allocations.
		memcpy(attached->data, otherAttached->constData, attached->size);
	}

	return true;
}


bool
DelayedMessageData::IsValid() const
{
	return fValid;
}


status_t
DelayedMessageData::Attach(const void* data, size_t size)
{
	// Sanity checking already performed
	Attachment* attach = new(std::nothrow) Attachment(data, size);

	if (attach == NULL)
		return B_NO_MEMORY;

	if (fAttachments.AddItem(attach) == false) {
		delete attach;
		return B_ERROR;
	}

	return B_OK;
}


bool
DelayedMessageData::Compare(Attachment* one, Attachment* two, int32 index)
{
	if (fMergeMode == DM_MERGE_DUPLICATES) {

		// Default-policy: all data must match
		if (fMergeMask == DM_DATA_DEFAULT || (fMergeMask & 1 << index) != 0)
			return memcmp(one->data, two->constData, one->size) == 0;

	} else if (fMergeMode == DM_MERGE_REPLACE) {

		// Default Policy: no data needs to match
		if (fMergeMask != DM_DATA_DEFAULT && (fMergeMask & 1 << index) != 0)
			return memcmp(one->data, two->constData, one->size) == 0;
	}

	return true;
}


void
DelayedMessageData::SetMerge(DMMergeMode mode, uint32 mask)
{
	fMergeMode = mode;
	fMergeMask = mask;
}


void
DelayedMessageData::SendFailed(port_id port)
{
	if (fFailureCallback != NULL)
		fFailureCallback(fCode, port, fFailureData);
}


void
DelayedMessageData::SetFailureCallback(FailureCallback callback, void* data)
{
	fFailureCallback = callback;
	fFailureData = data;
}


// #pragma mark -


ScheduledMessage::ScheduledMessage(DelayedMessage& message)
	:
	fData(message.HandOff())
{
}


ScheduledMessage::~ScheduledMessage()
{
	delete fData;
}


int32
ScheduledMessage::CountTargets() const
{
	if (fData == NULL)
		return 0;

	return fData->CountTargets();
}


bigtime_t
ScheduledMessage::ScheduledTime() const
{
	if (fData == NULL)
		return 0;

	return fData->ScheduledTime();
}


//! Send our message and data to their intended target(s)
int32
ScheduledMessage::SendMessage()
{
	if (fData == NULL || !fData->IsValid())
		return 0;

	int32 sent = 0;
	for (int32 index = 0; index < fData->Targets().CountItems(); ++index) {
		port_id port = *(fData->Targets().ItemAt(index));
		status_t error = SendMessageToPort(port);

		if (error == B_OK) {
			++sent;
			continue;
		}

		if (error != B_TIMED_OUT)
			fData->SendFailed(port);
	}

	return sent;
}


status_t
ScheduledMessage::SendMessageToPort(port_id port)
{
	if (fData == NULL || !fData->IsValid())
		return B_BAD_DATA;

	if (port == B_BAD_PORT_ID)
		return B_BAD_VALUE;

	BPrivate::LinkSender sender(port);
	if (sender.StartMessage(fData->Code()) != B_OK)
		return B_ERROR;

	AttachmentList& list = fData->Attachments();
	Attachment* attached = NULL;
	status_t error = B_OK;

	// The data has been checked already, so we assume it is all good
	for (int32 index = 0; index < list.CountItems(); ++index) {
		attached = list.ItemAt(index);

		error = sender.Attach(attached->data, attached->size);
		if (error != B_OK) {
			sender.CancelMessage();
			return error;
		}
	}

	// We do not want to ever hold up the sender thread for too long, we
	// set a 1 second sending delay, which should be more than enough for
	// 99.992% of all cases.  Approximately.
	error = sender.Flush(1000000);

	if (error == B_OK || error == B_BAD_PORT_ID)
		fData->RemoveTarget(port);

	return error;
}


bool
ScheduledMessage::IsValid() const
{
	return fData != NULL && fData->IsValid();
}


bool
ScheduledMessage::Merge(DelayedMessage& other)
{
	if (!IsValid())
		return false;

	return fData->MergeData(other.Data());
}


bool
ScheduledMessage::operator<(const ScheduledMessage& other) const
{
	if (!IsValid() || !other.IsValid())
		return false;

	return fData->ScheduledTime() < other.fData->ScheduledTime();
}


int
CompareMessages(const ScheduledMessage* one, const ScheduledMessage* two)
{
	return *one < *two;
}


// #pragma mark -


DelayedMessageSender::DelayedMessageSender()
	:
	fLock("DelayedMessageSender"),
	fMessages(20, true),
	fScheduledWakeup(B_INFINITE_TIMEOUT),
	fWakeupRetry(0),
	fThread(spawn_thread(&_thread_func, kName, kPriority, this)),
	fPort(create_port(kPortCapacity, "DelayedMessageSender")),
	fSentCount(0)
{
	resume_thread(fThread);
}


DelayedMessageSender::~DelayedMessageSender()
{
	// write the exit message to our port
	write_port(fPort, kExitMessage, NULL, 0);

	status_t status = B_OK;
	while (wait_for_thread(fThread, &status) == B_OK);

	// We now know the thread has exited, it is safe to cleanup
	delete_port(fPort);
}


status_t
DelayedMessageSender::ScheduleMessage(DelayedMessage& message)
{
	BAutolock _(fLock);

	// Can we merge with a pending message?
	ScheduledMessage* pending = NULL;
	for (int32 index = 0; index < fMessages.CountItems(); ++index) {
		pending = fMessages.ItemAt(index);
		if (pending->Merge(message))
			return B_OK;
	}

	// Guess not, add it to our list!
	ScheduledMessage* scheduled = new(std::nothrow) ScheduledMessage(message);

	if (scheduled == NULL)
		return B_NO_MEMORY;

	if (!scheduled->IsValid()) {
		delete scheduled;
		return B_BAD_DATA;
	}

	if (fMessages.AddItem(scheduled)) {
		fMessages.SortItems(&CompareMessages);
		_Wakeup(scheduled->ScheduledTime());
		return B_OK;
	}

	return B_ERROR;
}


int32
DelayedMessageSender::CountDelayedMessages() const
{
	BAutolock _(fLock);
	return fMessages.CountItems();
}


int64
DelayedMessageSender::CountSentMessages() const
{
	return atomic_get64(&fSentCount);
}


void
DelayedMessageSender::_MessageLoop()
{
	int32 code = -1;
	status_t status = B_TIMED_OUT;
	bigtime_t timeout = B_INFINITE_TIMEOUT;

	while (true) {
		timeout = atomic_get64(&fScheduledWakeup);
		if (timeout != B_INFINITE_TIMEOUT)
			timeout -= (system_time() + (DM_MINIMUM_DELAY / 2));

		if (timeout > DM_MINIMUM_DELAY / 4) {
			status = read_port_etc(fPort, &code, NULL, 0, B_RELATIVE_TIMEOUT,
				timeout);
		} else
			status = B_TIMED_OUT;

		if (status == B_INTERRUPTED)
			continue;

		if (status == B_TIMED_OUT) {
			_SendDelayedMessages();
			continue;
		}

		if (status == B_OK) {
			switch (code) {
				case kWakeupMessage:
					continue;

				case kExitMessage:
					return;

				// TODO: trace unhandled messages
				default:
					continue;
			}
		}

		// port deleted?
		if (status < B_OK)
			break;
	}
}


int32
DelayedMessageSender::_thread_func(void* sender)
{
	(static_cast<DelayedMessageSender*>(sender))->_MessageLoop();
	return 0;
}


//! Sends pending messages, call ONLY from sender thread!
int32
DelayedMessageSender::_SendDelayedMessages()
{
	// avoid sending messages during times of contention
	if (fLock.LockWithTimeout(30000) != B_OK) {
		atomic_add64(&fScheduledWakeup, DM_MINIMUM_DELAY);
		return 0;
	}

	atomic_set64(&fScheduledWakeup, B_INFINITE_TIMEOUT);

	if (fMessages.CountItems() == 0) {
		fLock.Unlock();
		return 0;
	}

	int32 sent = 0;

	bigtime_t time = system_time() + DM_MINIMUM_DELAY / 2;
		// capture any that may be on the verge of being sent.

	BObjectList<ScheduledMessage> remove;

	ScheduledMessage* message = NULL;
	for (int32 index = 0; index < fMessages.CountItems(); ++index) {
		message = fMessages.ItemAt(index);

		if (message->ScheduledTime() > time) {
			atomic_set64(&fScheduledWakeup, message->ScheduledTime());
			break;
		}

		int32 sendCount = message->SendMessage();
		if (sendCount > 0)
			sent += sendCount;

		if (message->CountTargets() == 0)
			remove.AddItem(message);
	}

	// remove serviced messages
	for (int32 index = 0; index < remove.CountItems(); ++index)
		fMessages.RemoveItem(remove.ItemAt(index));

	atomic_add64(&fSentCount, sent);

	// catch any partly-failed messages (possibly late):
	if (fMessages.CountItems() > 0
		&& atomic_get64(&fScheduledWakeup) == B_INFINITE_TIMEOUT) {

		fMessages.SortItems(&CompareMessages);
		message = fMessages.ItemAt(0);
		bigtime_t timeout = message->ScheduledTime() - time;

		if (timeout < 0)
			timeout = DM_MINIMUM_DELAY;

		atomic_set64(&fScheduledWakeup, timeout);
	}

	fLock.Unlock();
	return sent;
}


void
DelayedMessageSender::_Wakeup(bigtime_t when)
{
	if (atomic_get64(&fScheduledWakeup) < when
		&& atomic_get(&fWakeupRetry) == 0)
		return;

	atomic_set64(&fScheduledWakeup, when);

	BPrivate::LinkSender sender(fPort);
	sender.StartMessage(kWakeupMessage);
	status_t error = sender.Flush(30000);
	atomic_set(&fWakeupRetry, (int32)error == B_TIMED_OUT);
}

