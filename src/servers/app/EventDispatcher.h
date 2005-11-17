/*
 * Copyright 2005, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef EVENT_DISPATCHER_H
#define EVENT_DISPATCHER_H


#include <Locker.h>
#include <Messenger.h>

class BMessageFilter;

class EventStream;
class HWInterface;


class EventDispatcher : public BLocker {
	public:
		EventDispatcher();
		~EventDispatcher();

		status_t SetTo(EventStream& stream);
		status_t InitCheck();

		void SetFocus(BMessenger* messenger);

		void SetMouseFilter(BMessageFilter* filter);
		void SetKeyFilter(BMessageFilter* filter);

		bool HasCursorThread();
		void SetHWInterface(HWInterface* interface);

		EventStream* Stream();

	private:
		status_t _Run();
		void _Unset();

		void _SendMessage(BMessenger& messenger, BMessage* message, float importance);
		void _SetTransit(BMessage* message, int32 transit);

		void _EventLoop();
		void _CursorLoop();

		static status_t _event_looper(void* dispatcher);
		static status_t _cursor_looper(void* dispatcher);

	private:
		EventStream*	fStream;
		thread_id		fThread;
		thread_id		fCursorThread;

		BMessenger		fFocus;
		BMessenger		fLastFocus;
		bool			fHasFocus;
		bool			fHasLastFocus;
		bool			fTransit;
		BMessageFilter*	fMouseFilter;
		BMessageFilter*	fKeyFilter;

		BLocker			fCursorLock;
		HWInterface*	fHWInterface;
};

#endif	/* EVENT_DISPATCHER_H */
