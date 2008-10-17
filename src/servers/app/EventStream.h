/*
 * Copyright 2005, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef EVENT_STREAM_H
#define EVENT_STREAM_H


#include <LinkReceiver.h>
#include <MessageQueue.h>
#include <Messenger.h>


struct shared_cursor;


class EventStream {
	public:
		EventStream();
		virtual ~EventStream();

		virtual bool IsValid() = 0;
		virtual void SendQuit() = 0;

		virtual bool SupportsCursorThread() const;

		virtual void UpdateScreenBounds(BRect bounds) = 0;

		virtual bool GetNextEvent(BMessage** _event) = 0;
		virtual bool GetNextCursorPosition(BPoint& where);

		virtual status_t InsertEvent(BMessage* event) = 0;

		virtual BMessage* PeekLatestMouseMoved() = 0;
};


class InputServerStream : public EventStream {
	public:
		InputServerStream(BMessenger& inputServerMessenger);
#if TEST_MODE
		InputServerStream();
#endif

		virtual ~InputServerStream();

		virtual bool IsValid();
		virtual void SendQuit();

		virtual bool SupportsCursorThread() const { return fCursorSemaphore >= B_OK; }

		virtual void UpdateScreenBounds(BRect bounds);

		virtual bool GetNextEvent(BMessage** _event);
		virtual bool GetNextCursorPosition(BPoint& where);

		virtual status_t InsertEvent(BMessage* event);

		virtual BMessage* PeekLatestMouseMoved();

	private:
		status_t _MessageFromPort(BMessage** _message,
			bigtime_t timeout = B_INFINITE_TIMEOUT);

		BMessenger fInputServer;
		BMessageQueue fEvents;
		port_id	fPort;
		bool	fQuitting;
		sem_id	fCursorSemaphore;
		area_id	fCursorArea;
		shared_cursor* fCursorBuffer;
		BMessage* fLatestMouseMoved;
};

#endif	/* EVENT_STREAM_H */
