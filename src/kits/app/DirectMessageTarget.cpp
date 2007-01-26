/*
 * Copyright 2007, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <DirectMessageTarget.h>


namespace BPrivate {


BDirectMessageTarget::BDirectMessageTarget()
	:
	fReferenceCount(1),
	fClosed(false)
{
}


BDirectMessageTarget::~BDirectMessageTarget()
{
}


bool
BDirectMessageTarget::AddMessage(BMessage* message)
{
	if (fClosed) {
		delete message;
		return false;
	}

	fQueue.AddMessage(message);
	return true;
}


void
BDirectMessageTarget::Close()
{
	fClosed = true;
}


void
BDirectMessageTarget::Acquire()
{
	atomic_add(&fReferenceCount, 1);
}


void
BDirectMessageTarget::Release()
{
	if (atomic_add(&fReferenceCount, -1) == 1)
		delete this;
}

}	// namespace BPrivate
