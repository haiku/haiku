/*
 * Copyright 2007, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef _DIRECT_MESSAGE_TARGET_H
#define _DIRECT_MESSAGE_TARGET_H


#include <MessageQueue.h>


namespace BPrivate {

class BDirectMessageTarget {
	public:
		BDirectMessageTarget();

		bool AddMessage(BMessage* message);

		void Close();
		void Acquire();
		void Release();

		BMessageQueue* Queue() { return &fQueue; }

	private:
		~BDirectMessageTarget();
		
		int32			fReferenceCount;
		BMessageQueue	fQueue;
		bool			fClosed;
};

}	// namespace BPrivate

#endif	// _DIRECT_MESSAGE_TARGET_H
