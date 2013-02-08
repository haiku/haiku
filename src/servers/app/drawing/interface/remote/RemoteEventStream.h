/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */
#ifndef REMOTE_EVENT_STREAM_H
#define REMOTE_EVENT_STREAM_H

#include "EventStream.h"

#include <Locker.h>
#include <ObjectList.h>

class RemoteMessage;

class RemoteEventStream : public EventStream {
public:
								RemoteEventStream();
virtual							~RemoteEventStream();

virtual	bool					IsValid() { return true; }
virtual	void					SendQuit() {}

virtual	void					UpdateScreenBounds(BRect bounds);
virtual	bool					GetNextEvent(BMessage** _event);
virtual	status_t				InsertEvent(BMessage* event);
virtual	BMessage*				PeekLatestMouseMoved();

		bool					EventReceived(RemoteMessage& message);

private:
		BObjectList<BMessage>	fEventList;
		BLocker					fEventListLocker;
		sem_id					fEventNotification;
		bool					fWaitingOnEvent;
		BMessage*				fLatestMouseMovedEvent;

		BPoint					fMousePosition;
		uint32					fMouseButtons;
		uint32					fModifiers;
};

#endif // REMOTE_EVENT_STREAM_H
