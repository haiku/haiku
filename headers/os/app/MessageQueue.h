/*
 * Copyright 2001-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_MESSAGE_QUEUE_H
#define	_MESSAGE_QUEUE_H


#include <Locker.h>
#include <Message.h>
	/* For convenience */


class BMessageQueue {
	public:
		BMessageQueue();
		virtual ~BMessageQueue();

		void AddMessage(BMessage* message);
		void RemoveMessage(BMessage* message);

		int32 CountMessages() const;
		bool IsEmpty() const;

		BMessage* FindMessage(int32 index) const;
		BMessage* FindMessage(uint32 what, int32 index = 0) const;

		bool Lock();
		void Unlock();
		bool IsLocked() const;

		BMessage *NextMessage();
		bool IsNextMessage(const BMessage* message) const;

	private:
		// Reserved space in the vtable for future changes to BMessageQueue
		virtual void _ReservedMessageQueue1();
		virtual void _ReservedMessageQueue2();
		virtual void _ReservedMessageQueue3();

		BMessageQueue(const BMessageQueue &);
		BMessageQueue &operator=(const BMessageQueue &);

		bool IsLocked();
			// this needs to be exported for R5 compatibility and should
			// be dropped as soon as possible

	private:	
		BMessage* fHead;
		BMessage* fTail;
		int32 fMessageCount;
		mutable BLocker fLock;

		uint32 _reserved[3];
};

#endif	// _MESSAGE_QUEUE_H
