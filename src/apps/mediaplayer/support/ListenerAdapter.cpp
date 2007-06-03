/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#include "ListenerAdapter.h"

#include <Message.h>

// constructor
ListenerAdapter::ListenerAdapter(BHandler* handler)
	: Listener()
	, AbstractLOAdapter(handler)
{
}

// destructor
ListenerAdapter::~ListenerAdapter()
{
}

// ObjectChanged
void
ListenerAdapter::ObjectChanged(const Notifier* object)
{
	BMessage message(MSG_OBJECT_CHANGED);
	message.AddPointer("object", object);

	DeliverMessage(message);
}
