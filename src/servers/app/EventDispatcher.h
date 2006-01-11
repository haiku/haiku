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
#include <Message.h>
#include <MessageFilter.h>
#include <Messenger.h>
#include <ObjectList.h>


class EventStream;
class HWInterface;

struct event_listener;


class EventTarget {
	public:
		EventTarget();
		~EventTarget();

		void SetTo(const BMessenger& messenger);
		BMessenger& Messenger() { return fMessenger; }

		event_listener* FindListener(int32 token, int32* _index = NULL);
		bool AddListener(int32 token, uint32 eventMask, uint32 options,
				bool temporary);
		void RemoveListener(event_listener* listener, bool temporary);

		bool RemoveListener(int32 token);
		bool RemoveTemporaryListener(int32 token);
		void RemoveTemporaryListeners();

		bool IsEmpty() const { return fListeners.IsEmpty(); }

		int32 CountListeners() const { return fListeners.CountItems(); }
		event_listener* ListenerAt(int32 index) const
				{ return fListeners.ItemAt(index); }

	private:
		bool _RemoveTemporaryListener(event_listener* listener, int32 index);

		BObjectList<event_listener> fListeners;
		BMessenger	fMessenger;
};

class EventFilter {
	public:
		virtual filter_result Filter(BMessage* event, EventTarget** _target,
			int32* _viewToken = NULL, BMessage* latestMouseMoved = NULL) = 0;
};

class EventDispatcher : public BLocker {
	public:
		EventDispatcher();
		~EventDispatcher();

		status_t SetTo(EventStream* stream);
		status_t InitCheck();

		void RemoveTarget(EventTarget& target);

		bool AddListener(EventTarget& target, int32 token,
				uint32 eventMask, uint32 options);
		bool AddTemporaryListener(EventTarget& target,
				int32 token, uint32 eventMask, uint32 options);
		void RemoveListener(EventTarget& target, int32 token);
		void RemoveTemporaryListener(EventTarget& target, int32 token);

		void SetMouseFilter(EventFilter* filter);
		void SetKeyboardFilter(EventFilter* filter);

		void GetMouse(BPoint& where, int32& buttons);
		void SendFakeMouseMoved(EventTarget& target, int32 viewToken);

		bool HasCursorThread();
		void SetHWInterface(HWInterface* interface);

		void SetDragMessage(BMessage& message);
			// the message should be delivered on the next
			// "mouse up".
			// if the mouse is not pressed, it should
			// be delivered to the "current" target right away.

	private:
		status_t _Run();
		void _Unset();

		bool _SendMessage(BMessenger& messenger, BMessage* message, float importance);

		bool _AddTokens(BMessage* message, EventTarget* target, uint32 eventMask,
				BMessage* nextMouseMoved = NULL, int32* _viewToken = NULL);
		void _RemoveTokens(BMessage* message);
		void _SetFeedFocus(BMessage* message);
		void _UnsetFeedFocus(BMessage* message);

		void _SetMouseTarget(const BMessenger* messenger);
		void _UnsetLastMouseTarget();

		bool _AddListener(EventTarget& target, int32 token,
				uint32 eventMask, uint32 options, bool temporary);
		void _RemoveTemporaryListeners();

		void _DeliverDragMessage();

		void _EventLoop();
		void _CursorLoop();

		static status_t _event_looper(void* dispatcher);
		static status_t _cursor_looper(void* dispatcher);

	private:
		EventStream*	fStream;
		thread_id		fThread;
		thread_id		fCursorThread;

		EventTarget*	fPreviousMouseTarget;
		EventTarget*	fFocus;
		bool			fSuspendFocus;

		EventFilter*	fMouseFilter;
		EventFilter*	fKeyboardFilter;

		BObjectList<EventTarget> fTargets;

		BMessage*		fNextLatestMouseMoved;
		BPoint			fLastCursorPosition;
		int32			fLastButtons;

		BMessage		fDragMessage;
		bool			fDraggingMessage;

		BLocker			fCursorLock;
		HWInterface*	fHWInterface;
};

#endif	/* EVENT_DISPATCHER_H */
