/*
 * Copyright 2005, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "EventDispatcher.h"
#include "EventStream.h"
#include "HWInterface.h"

#include <Autolock.h>
#include <MessageFilter.h>
#include <View.h>

#include <stdio.h>


static const float kMouseMovedImportance = 0.1f;
static const float kMouseTransitImportance = 1.0f;
static const float kStandardImportance = 0.9f;


EventDispatcher::EventDispatcher()
	: BLocker("event dispatcher"),
	fStream(NULL),
	fThread(-1),
	fCursorThread(-1),
	fHasFocus(false),
	fHasLastFocus(false),
	fTransit(false),
	fMouseFilter(NULL),
	fKeyFilter(NULL),
	fCursorLock("cursor loop lock"),
	fHWInterface(NULL)
{
}


EventDispatcher::~EventDispatcher()
{
	_Unset();
}


status_t
EventDispatcher::SetTo(EventStream& stream)
{
	_Unset();

	fStream = &stream;
	return _Run();
}


status_t
EventDispatcher::InitCheck()
{
	if (fStream != NULL) {
		if (fThread < B_OK)
			return fThread;

		return B_OK;
	}
	return B_NO_INIT;
}


void
EventDispatcher::_Unset()
{
	fStream->SendQuit();

	wait_for_thread(fThread, NULL);
	wait_for_thread(fCursorThread, NULL);

	fThread = fCursorThread = -1;
}


status_t
EventDispatcher::_Run()
{
	fThread = spawn_thread(_event_looper, "event loop",
		B_REAL_TIME_DISPLAY_PRIORITY - 10, this);
	if (fThread < B_OK)
		return fThread;

	if (fStream->SupportsCursorThread()) {
		fCursorThread = spawn_thread(_cursor_looper, "cursor loop",
			B_REAL_TIME_DISPLAY_PRIORITY - 5, this);
		if (resume_thread(fCursorThread) != B_OK) {
			kill_thread(fCursorThread);
			fCursorThread = -1;
		}
	}

	return resume_thread(fThread);
}


void
EventDispatcher::SetFocus(BMessenger* messenger)
{
	BAutolock _(this);

	if (fFocus == *messenger)
		return;

	fHasLastFocus = fHasFocus;
	if (fHasFocus)
		fLastFocus = fFocus;

	fHasFocus = messenger != NULL;
	if (fHasFocus)
		fFocus = *messenger;

	fTransit = true;
}


void
EventDispatcher::SetMouseFilter(BMessageFilter* filter)
{
	BAutolock _(this);

	if (fMouseFilter == filter)
		return;

	delete fMouseFilter;
	fMouseFilter = filter;
}


void
EventDispatcher::SetKeyFilter(BMessageFilter* filter)
{
	BAutolock _(this);

	if (fKeyFilter == filter)
		return;

	delete fKeyFilter;
	fKeyFilter = filter;
}


bool
EventDispatcher::HasCursorThread()
{
	return fCursorThread >= B_OK;
}


/*!
	\brief Sets the HWInterface to use when moving the mouse cursor.
		\a interface is allowed to be NULL.
*/
void
EventDispatcher::SetHWInterface(HWInterface* interface)
{
	BAutolock _(fCursorLock);

	fHWInterface = interface;
}


void
EventDispatcher::_SendMessage(BMessenger& messenger, BMessage* message,
	float importance)
{
	// TODO: add failed messages to a queue, and start dropping them by importance

	status_t status = messenger.SendMessage(message, (BHandler*)NULL, 100000);
	if (status != B_OK)
		printf("failed to send message to target: %lx\n", message->what);
}


void
EventDispatcher::_SetTransit(BMessage* message, int32 transit)
{
	if (message->ReplaceInt32("be:transit", transit) != B_OK)
		message->AddInt32("be:transit", transit);
}


void
EventDispatcher::_EventLoop()
{
	BMessage* event;
	while (fStream->GetNextEvent(&event)) {
		if (event == NULL) {
			// may happen in out of memory situations or junk at the port
			// we can't do anything about those yet
			continue;
		}

		BAutolock _(this);
		bool sendToLastFocus = false;

		switch (event->what) {
			case B_MOUSE_MOVED:
				if (!HasCursorThread()) {
					// there is no cursor thread, we need to move the cursor ourselves
					BAutolock _(fCursorLock);

					BPoint where;
					if (fHWInterface != NULL && event->FindPoint("where", &where) == B_OK)
						fHWInterface->MoveCursorTo(where.x, where.y);
				}

				if (fTransit) {
					// target has changed, we need to add the be:transit field
					// to the message
					if (fHasLastFocus) {
						_SetTransit(event, B_EXITED_VIEW);
						_SendMessage(fLastFocus, event, kMouseTransitImportance);
						sendToLastFocus = true;

						// we no longer need the last focus messenger
						fHasLastFocus = false;
					}
					if (fHasFocus)
						_SetTransit(event, B_ENTERED_VIEW);
				}
				// supposed to fall through
			case B_MOUSE_DOWN:
			case B_MOUSE_UP:
				if (fMouseFilter != NULL
					&& fMouseFilter->Filter(event, NULL) == B_SKIP_MESSAGE)
					break;

				if (fHasFocus) {
					_SendMessage(fFocus, event, event->what == B_MOUSE_MOVED
						? kMouseMovedImportance : kStandardImportance);
				}
				break;

			case B_KEY_DOWN:
			case B_KEY_UP:
			case B_UNMAPPED_KEY_DOWN:
			case B_UNMAPPED_KEY_UP:
			case B_MODIFIERS_CHANGED:
				if (fKeyFilter != NULL
					&& fKeyFilter->Filter(event, NULL) == B_SKIP_MESSAGE)
					break;

			default:
				if (fHasFocus)
					_SendMessage(fFocus, event, kStandardImportance);
				break;
		}

		delete event;
	}
}


void
EventDispatcher::_CursorLoop()
{
	BPoint where;
	while (fStream->GetNextCursorPosition(where)) {
		BAutolock _(fCursorLock);

		if (fHWInterface != NULL)
			fHWInterface->MoveCursorTo(where.x, where.y);
	}

	fCursorThread = -1;
}


/*static*/
status_t
EventDispatcher::_event_looper(void* _dispatcher)
{
	EventDispatcher* dispatcher = (EventDispatcher*)_dispatcher;

	dispatcher->_EventLoop();
	return B_OK;
}


/*static*/
status_t
EventDispatcher::_cursor_looper(void* _dispatcher)
{
	EventDispatcher* dispatcher = (EventDispatcher*)_dispatcher;

	dispatcher->_CursorLoop();
	return B_OK;
}
