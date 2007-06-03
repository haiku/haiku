/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */

#include "AbstractLOAdapter.h"

#include <Handler.h>
#include <Looper.h>
#include <Messenger.h>

// constructor
AbstractLOAdapter::AbstractLOAdapter(BHandler* handler)
	: fHandler(handler),
	  fMessenger(NULL)
{
}

// constructor
AbstractLOAdapter::AbstractLOAdapter(const BMessenger& messenger)
	: fHandler(NULL),
	  fMessenger(new BMessenger(messenger))
{
}

// destructor
AbstractLOAdapter::~AbstractLOAdapter()
{
	delete fMessenger;
}

// DeliverMessage
void
AbstractLOAdapter::DeliverMessage(BMessage* message)
{
	if (fHandler) {
		if (BLooper* looper = fHandler->Looper())
			looper->PostMessage(message, fHandler);
	} else if (fMessenger)
		fMessenger->SendMessage(message);
}

// DeliverMessage
void
AbstractLOAdapter::DeliverMessage(BMessage& message)
{
	DeliverMessage(&message);
}

// DeliverMessage
void
AbstractLOAdapter::DeliverMessage(uint32 command)
{
	BMessage message(command);
	DeliverMessage(&message);
}

