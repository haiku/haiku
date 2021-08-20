//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//---------------------------------------------------------------------

#include <Message.h>

#include "PriorityMessageQueue.h"

// MessageInfo
class PriorityMessageQueue::MessageInfo {
public:
	MessageInfo(BMessage *message, int32 priority)
		: fMessage(message),
		  fPriority(priority)
	{
	}

	BMessage *Message() const	{ return fMessage; }
	int32 Priority() const		{ return fPriority; }

private:
	BMessage	*fMessage;
	int32		fPriority;
};


// constructor
PriorityMessageQueue::PriorityMessageQueue()
	: fLock(),
	  fMessages(20, true)
{
}

// destructor
PriorityMessageQueue::~PriorityMessageQueue()
{
	// delete the messages
	for (int32 i = 0; MessageInfo *info = fMessages.ItemAt(i); i++)
		delete info->Message();
	// the infos are deleted automatically
}

// Lock
bool
PriorityMessageQueue::Lock()
{
	return fLock.Lock();
}

// Unlock
void
PriorityMessageQueue::Unlock()
{
	fLock.Unlock();
}

// PushMessage
bool
PriorityMessageQueue::PushMessage(BMessage *message, int32 priority)
{
	bool result = (message);
	if (result)
		result = Lock();
	if (result) {
		if (MessageInfo *info = new MessageInfo(message, priority)) {
			// find the insertion index
			int32 index = _FindInsertionIndex(priority);
			if (!fMessages.AddItem(info, index)) {
				result = false;
				delete info;
			}
		} else	// no memory
			result = false;
		Unlock();
	}
	return result;
}

// PopMessage
BMessage *
PriorityMessageQueue::PopMessage()
{
	BMessage *result = NULL;
	if (Lock()) {
		if (MessageInfo *info = fMessages.RemoveItemAt(0)) {
			result = info->Message();
			delete info;
		}
		Unlock();
	}
	return result;
}

// CountMessages
int32
PriorityMessageQueue::CountMessages() const
{
	int32 result = 0;
	if (fLock.Lock()) {
		result = fMessages.CountItems();
		fLock.Unlock();
	}
	return result;
}

// IsEmpty
bool
PriorityMessageQueue::IsEmpty() const
{
	return (CountMessages() == 0);
}

// _FindInsertionIndex
int32
PriorityMessageQueue::_FindInsertionIndex(int32 priority)
{
	int32 lower = 0;
	int32 upper = fMessages.CountItems();
	while (lower < upper) {
		int32 mid = (lower + upper) / 2;
		MessageInfo *info = fMessages.ItemAt(mid);
		if (info->Priority() >= priority)
			lower = mid + 1;
		else
			upper = mid;
	}
	return lower;
}

