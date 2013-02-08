/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */
#ifndef CANVAS_EVENT_STREAM_H
#define CANVAS_EVENT_STREAM_H

#include "EventStream.h"

#include <Locker.h>
#include <ObjectList.h>

class CanvasMessage;

class CanvasEventStream : public EventStream {
public:
								CanvasEventStream();
virtual							~CanvasEventStream();

virtual	bool					IsValid() { return true; }
virtual	void					SendQuit() {}

virtual	void					UpdateScreenBounds(BRect bounds);
virtual	bool					GetNextEvent(BMessage** _event);
virtual	status_t				InsertEvent(BMessage* event);
virtual	BMessage*				PeekLatestMouseMoved();

		bool					EventReceived(CanvasMessage& message);

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

#endif // CANVAS_EVENT_STREAM_H
