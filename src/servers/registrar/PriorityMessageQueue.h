//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//---------------------------------------------------------------------

#ifndef PRIORITY_MESSAGE_QUEUE_H
#define PRIORITY_MESSAGE_QUEUE_H

#include <Locker.h>
#include <ObjectList.h>

class BMessage;

class PriorityMessageQueue {
public:
	PriorityMessageQueue();
	~PriorityMessageQueue();

	bool Lock();
	void Unlock();

	bool PushMessage(BMessage *message, int32 priority = 0);
	BMessage *PopMessage();

	int32 CountMessages() const;
	bool IsEmpty() const;

private:
	int32 _FindInsertionIndex(int32 priority);

private:
	class MessageInfo;

private:
	mutable BLocker				fLock;
	BObjectList<MessageInfo>	fMessages;
};

#endif	// PRIORITY_MESSAGE_QUEUE_H
