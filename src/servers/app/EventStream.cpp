/*
 * Copyright 2005, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "EventStream.h"

#include <InputServerTypes.h>
#include <ServerProtocol.h>
#include <shared_cursor_area.h>

#include <AppMisc.h>
#include <AutoDeleter.h>

#include <new>
#include <stdio.h>
#include <string.h>


EventStream::EventStream()
{
}


EventStream::~EventStream()
{
}


bool
EventStream::SupportsCursorThread() const
{
	return false;
}


status_t
EventStream::GetNextCursorPosition(BPoint& where, bigtime_t timeout)
{
	return B_ERROR;
}


//	#pragma mark -


InputServerStream::InputServerStream(BMessenger& messenger)
	:
	fInputServer(messenger),
	fPort(-1),
	fQuitting(false),
	fLatestMouseMoved(NULL)
{
	BMessage message(IS_ACQUIRE_INPUT);
	message.AddInt32("remote team", BPrivate::current_team());

	fCursorArea = create_area("shared cursor", (void **)&fCursorBuffer, B_ANY_ADDRESS,
		B_PAGE_SIZE, B_LAZY_LOCK, B_READ_AREA | B_WRITE_AREA | B_CLONEABLE_AREA);
	if (fCursorArea >= B_OK)
		message.AddInt32("cursor area", fCursorArea);

	BMessage reply;
	if (messenger.SendMessage(&message, &reply) != B_OK)
		return;

	if (reply.FindInt32("event port", &fPort) != B_OK)
		fPort = -1;
	if (reply.FindInt32("cursor semaphore", &fCursorSemaphore) != B_OK)
		fCursorSemaphore = -1;
}


#if TEST_MODE
InputServerStream::InputServerStream()
	:
	fQuitting(false),
	fCursorSemaphore(-1),
	fLatestMouseMoved(NULL)
{
	fPort = find_port(SERVER_INPUT_PORT);
}
#endif


InputServerStream::~InputServerStream()
{
	delete_area(fCursorArea);
}


bool
InputServerStream::IsValid()
{
	port_info portInfo;
	if (fPort < B_OK || get_port_info(fPort, &portInfo) != B_OK)
		return false;

	return true;
}


void
InputServerStream::SendQuit()
{
	fQuitting = true;
	write_port(fPort, 'quit', NULL, 0);
	release_sem(fCursorSemaphore);
}


void
InputServerStream::UpdateScreenBounds(BRect bounds)
{
	BMessage update(IS_SCREEN_BOUNDS_UPDATED);
	update.AddRect("screen_bounds", bounds);

	fInputServer.SendMessage(&update);
}


bool
InputServerStream::GetNextEvent(BMessage** _event)
{
	while (fEvents.IsEmpty()) {
		// wait for new events
		BMessage* event;
		status_t status = _MessageFromPort(&event);
		if (status == B_OK) {
			if (event->what == B_MOUSE_MOVED)
				fLatestMouseMoved = event;

			fEvents.AddMessage(event);
		} else if (status == B_BAD_PORT_ID) {
			// our port got deleted - the input_server must have died
			fPort = -1;
			return false;
		}

		int32 count = port_count(fPort);
		if (count > 0) {
			// empty port queue completely while we're at it
			for (int32 i = 0; i < count; i++) {
				if (_MessageFromPort(&event, 0) == B_OK) {
					if (event->what == B_MOUSE_MOVED)
						fLatestMouseMoved = event;
					fEvents.AddMessage(event);
				}
			}
		}
	}

	// there are items in our list, so just work through them

	*_event = fEvents.NextMessage();
	return true;
}


status_t
InputServerStream::GetNextCursorPosition(BPoint &where, bigtime_t timeout)
{
	status_t status;

	do {
		status = acquire_sem_etc(fCursorSemaphore, 1, B_RELATIVE_TIMEOUT,
			timeout);
	} while (status == B_INTERRUPTED);

	if (status == B_TIMED_OUT)
		return status;

	if (status == B_BAD_SEM_ID) {
		// the semaphore is no longer valid - the input_server must have died
		fCursorSemaphore = -1;
		return B_ERROR;
	}

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	uint32 pos = atomic_get((int32*)&fCursorBuffer->pos);
#else
	uint32 pos = fCursorBuffer->pos;
#endif

	where.x = pos >> 16UL;
	where.y = pos & 0xffff;

	atomic_and(&fCursorBuffer->read, 0);
		// this tells the input_server that we've read the
		// cursor position and want to be notified if updated

	if (fQuitting) {
		fQuitting = false;
		return B_ERROR;
	}

	return B_OK;
}


status_t
InputServerStream::InsertEvent(BMessage* event)
{
	fEvents.AddMessage(event);
	status_t status = write_port_etc(fPort, 'insm', NULL, 0, B_RELATIVE_TIMEOUT,
		0);
	if (status == B_BAD_PORT_ID)
		return status;

	// If the port is full, we obviously don't care to report this, as we
	// already placed our message.
	return B_OK;
}


BMessage*
InputServerStream::PeekLatestMouseMoved()
{
	return fLatestMouseMoved;
}


status_t
InputServerStream::_MessageFromPort(BMessage** _message, bigtime_t timeout)
{
	uint8 *buffer = NULL;
	ssize_t bufferSize;

	// read message from port

	do {
		bufferSize = port_buffer_size_etc(fPort, B_RELATIVE_TIMEOUT, timeout);
	} while (bufferSize == B_INTERRUPTED);

	if (bufferSize < B_OK)
		return bufferSize;

	if (bufferSize > 0) {
		buffer = new (std::nothrow) uint8[bufferSize];
		if (buffer == NULL)
			return B_NO_MEMORY;
	}

	int32 code;
	bufferSize = read_port_etc(fPort, &code, buffer, bufferSize,
		B_RELATIVE_TIMEOUT, 0);
	if (bufferSize < B_OK) {
		delete[] buffer;
		return bufferSize;
	}

	if (code == 'quit') {
		// this will cause GetNextEvent() to return false
		return B_BAD_PORT_ID;
	}
	if (code == 'insm') {
		// a message has been inserted into our queue
		return B_INTERRUPTED;
	}

	// we have the message, now let's unflatten it

	ObjectDeleter<BMessage> message(new BMessage(code));
	if (message.Get() == NULL)
		return B_NO_MEMORY;

	if (buffer == NULL) {
		*_message = message.Detach();
		return B_OK;
	}

	status_t status = message->Unflatten((const char*)buffer);
	delete[] buffer;

	if (status != B_OK) {
		printf("Unflatten event failed: %s, port message code was: %" B_PRId32
			" - %c%c%c%c\n", strerror(status), code, (int8)(code >> 24),
			(int8)(code >> 16), (int8)(code >> 8), (int8)code);
		return status;
	}

	*_message = message.Detach();
	return B_OK;
}

