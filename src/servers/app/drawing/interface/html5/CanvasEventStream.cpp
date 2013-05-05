/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include "CanvasEventStream.h"

#include "CanvasMessage.h"
#include "StreamingRingBuffer.h"

#include <Autolock.h>

#include <new>


CanvasEventStream::CanvasEventStream()
	:
	fEventList(10, true),
	fEventListLocker("canvas event list"),
	fEventNotification(-1),
	fWaitingOnEvent(false),
	fLatestMouseMovedEvent(NULL),
	fMousePosition(0, 0),
	fMouseButtons(0),
	fModifiers(0)
{
	fEventNotification = create_sem(0, "canvas event notification");
}


CanvasEventStream::~CanvasEventStream()
{
	delete_sem(fEventNotification);
}


void
CanvasEventStream::UpdateScreenBounds(BRect bounds)
{
}


bool
CanvasEventStream::GetNextEvent(BMessage** _event)
{
	BAutolock lock(fEventListLocker);
	while (fEventList.CountItems() == 0) {
		fWaitingOnEvent = true;
		lock.Unlock();

		status_t result;
		do {
			result = acquire_sem(fEventNotification);
		} while (result == B_INTERRUPTED);

		lock.Lock();
		if (!lock.IsLocked())
			return false;
	}

	*_event = fEventList.RemoveItemAt(0);
	return true;
}


status_t
CanvasEventStream::InsertEvent(BMessage* event)
{
	BAutolock lock(fEventListLocker);
	if (!lock.IsLocked())
		return B_ERROR;

	if (!fEventList.AddItem(event))
		return B_ERROR;

	if (event->what == B_MOUSE_MOVED)
		fLatestMouseMovedEvent = event;

	return B_OK;
}


BMessage*
CanvasEventStream::PeekLatestMouseMoved()
{
	return fLatestMouseMovedEvent;
}


bool
CanvasEventStream::EventReceived(CanvasMessage& message)
{
	uint16 code = message.Code();
	uint32 what = 0;
	switch (code) {
		case RP_MOUSE_MOVED:
			what = B_MOUSE_MOVED;
			break;
		case RP_MOUSE_DOWN:
			what = B_MOUSE_DOWN;
			break;
		case RP_MOUSE_UP:
			what = B_MOUSE_UP;
			break;
		case RP_MOUSE_WHEEL_CHANGED:
			what = B_MOUSE_WHEEL_CHANGED;
			break;
		case RP_KEY_DOWN:
			what = B_KEY_DOWN;
			break;
		case RP_KEY_UP:
			what = B_KEY_UP;
			break;
		case RP_MODIFIERS_CHANGED:
			what = B_MODIFIERS_CHANGED;
			break;
	}

	if (what == 0)
		return false;

	BMessage* event = new BMessage(what);
	if (event == NULL)
		return false;

	event->AddInt64("when", system_time());

	switch (code) {
		case RP_MOUSE_MOVED:
		case RP_MOUSE_DOWN:
		case RP_MOUSE_UP:
		{
			message.Read(fMousePosition);
			if (code != RP_MOUSE_MOVED)
				message.Read(fMouseButtons);

			event->AddPoint("where", fMousePosition);
			event->AddInt32("buttons", fMouseButtons);
			event->AddInt32("modifiers", fModifiers);

			if (code == RP_MOUSE_DOWN) {
				int32 clicks;
				if (message.Read(clicks) == B_OK)
					event->AddInt32("clicks", clicks);
			}

			if (code == RP_MOUSE_MOVED)
				fLatestMouseMovedEvent = event;
			break;
		}

		case RP_MOUSE_WHEEL_CHANGED:
		{
			float xDelta, yDelta;
			message.Read(xDelta);
			message.Read(yDelta);
			event->AddFloat("be:wheel_delta_x", xDelta);
			event->AddFloat("be:wheel_delta_y", yDelta);
			break;
		}

		case RP_KEY_DOWN:
		case RP_KEY_UP:
		{
			int32 numBytes;
			if (message.Read(numBytes) != B_OK)
				break;

			char* bytes = (char*)malloc(numBytes + 1);
			if (bytes == NULL)
				break;

			if (message.ReadList(bytes, numBytes) != B_OK) {
				free(bytes);
				break;
			}

			for (int32 i = 0; i < numBytes; i++)
				event->AddInt8("byte", (int8)bytes[i]);

			bytes[numBytes] = 0;
			event->AddData("bytes", B_STRING_TYPE, bytes, numBytes + 1, false);
			event->AddInt32("modifiers", fModifiers);

			int32 rawChar;
			if (message.Read(rawChar) == B_OK)
				event->AddInt32("raw_char", rawChar);

			int32 key;
			if (message.Read(key) == B_OK)
				event->AddInt32("key", key);

			free(bytes);
			break;
		}

		case RP_MODIFIERS_CHANGED:
		{
			event->AddInt32("be:old_modifiers", fModifiers);
			message.Read(fModifiers);
			event->AddInt32("modifiers", fModifiers);
			break;
		}
	}

	BAutolock lock(fEventListLocker);
	fEventList.AddItem(event);
	if (fWaitingOnEvent) {
		fWaitingOnEvent = false;
		lock.Unlock();
		release_sem(fEventNotification);
	}

	return true;
}
