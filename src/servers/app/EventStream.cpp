/*
 * Copyright 2005, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "EventStream.h"

#include <shared_cursor_area.h>

#include <new>
#include <stdio.h>


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


//	#pragma mark -


InputServerStream::InputServerStream(port_id port, port_id inputServerPort)
	:
	fPort(port),
	fQuitting(false)
{
}


InputServerStream::~InputServerStream()
{
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


bool
InputServerStream::GetNextEvent(BMessage** _event)
{
	if (fEvents.IsEmpty()) {
		// wait for new events
		BMessage* event;
		status_t status = _MessageFromPort(&event);
		if (status == B_OK)
			fEvents.AddMessage(event);
		else if (status == B_BAD_PORT_ID) {
			// our port got deleted - the input_server must have died
			fPort = -1;
			return false;
		}

		int32 count = port_count(fPort);
		if (count > 0) {
			// empty port queue completely while we're at it
			for (int32 i = 0; i < count; i++) {
				if (_MessageFromPort(&event, 0) == B_OK)
					fEvents.AddMessage(event);
			}
		}
	}

	// there are items in our list, so just work through them

	*_event = fEvents.NextMessage();
	return true;
}


bool
InputServerStream::GetNextCursorPosition(BPoint &where)
{
	status_t status;
	do {
		status = acquire_sem(fCursorSemaphore);
	} while (status == B_INTERRUPTED);

	if (status == B_BAD_SEM_ID) {
		// the semaphore is no longer valid - the input_server must have died
		fCursorSemaphore = -1;
		return false;
	}

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	uint32 pos = atomic_get((int32*)&fCursorBuffer->pos);
#else
	uint32 pos = fCursorBuffer->pos;
#endif

	where.x = pos & 0xffff;
	where.y = pos >> 16L;

	atomic_and(&fCursorBuffer->read, 0);
		// this tells the input_server that we've read the
		// cursor position and want to be notified if updated

	if (fQuitting) {
		fQuitting = false;
		return false;
	}

	return true;
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
		// special code to tell our client to quit
		BMessage* message = new BMessage(B_QUIT_REQUESTED);
		if (message == NULL)
			return B_NO_MEMORY;

		*_message = message;
		return B_OK;
	}

	// we have the message, now let's unflatten it

	BMessage* message = new BMessage(code);
	if (message == NULL)
		return B_NO_MEMORY;

	if (buffer == NULL) {
		*_message = message;
		return B_OK;
	}

	status_t status = message->Unflatten((const char*)buffer);
	delete[] buffer;

	if (status != B_OK) {
		printf("Unflatten event failed: port message code was: %ld - %c%c%c%c\n",
			code, (int8)(code >> 24), (int8)(code >> 16), (int8)(code >> 8), (int8)code);
		delete message;
		return status;
	}

	*_message = message;
	return B_OK;
}

