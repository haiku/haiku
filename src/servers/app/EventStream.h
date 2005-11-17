/*
 * Copyright 2005, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef EVENT_STREAM_H
#define EVENT_STREAM_H


#include <MessageQueue.h>

struct shared_cursor;


class EventStream {
	public:
		EventStream();
		virtual ~EventStream();

		virtual bool IsValid() = 0;
		virtual void SendQuit() = 0;

		virtual bool SupportsCursorThread() const;

		virtual bool GetNextEvent(BMessage** _event) = 0;
		virtual bool GetNextCursorPosition(BPoint& where) = 0;
};


class InputServerStream {
	public:
		InputServerStream(port_id port, port_id inputServerPort);
		virtual ~InputServerStream();

		virtual bool IsValid();
		virtual void SendQuit();

		virtual bool SupportsCursorThread() const { return fCursorSemaphore >= B_OK; }

		virtual bool GetNextEvent(BMessage** _event);
		virtual bool GetNextCursorPosition(BPoint& where);

	private:
		status_t _MessageFromPort(BMessage** _message,
			bigtime_t timeout = B_INFINITE_TIMEOUT);

		BMessageQueue fEvents;
		port_id	fPort;
		bool	fQuitting;
		sem_id	fCursorSemaphore;
		area_id	fCursorArea;
		shared_cursor* fCursorBuffer;
};

#endif	/* EVENT_STREAM_H */
