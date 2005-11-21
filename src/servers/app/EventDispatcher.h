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
#include <ObjectList.h>

class BMessageFilter;

class EventStream;
class HWInterface;


class EventDispatcher : public BLocker {
	public:
		EventDispatcher();
		~EventDispatcher();

		status_t SetTo(EventStream* stream);
		status_t InitCheck();

		void SetFocus(const BMessenger* messenger);

		bool AddListener(const BMessenger& messenger, int32 token,
				uint32 eventMask, uint32 options);
		bool AddTemporaryListener(const BMessenger& messenger,
				int32 token, uint32 eventMask, uint32 options);
		void RemoveListener(const BMessenger& messenger, int32 token);
		void RemoveTemporaryListener(const BMessenger& messenger, int32 token);

		void SetMouseFilter(BMessageFilter* filter);
		void SetKeyboardFilter(BMessageFilter* filter);

		bool HasCursorThread();
		void SetHWInterface(HWInterface* interface);

	private:
		struct event_target;

		status_t _Run();
		void _Unset();

		bool _SendMessage(BMessenger& messenger, BMessage* message, float importance);
		void _SetTransit(BMessage* message, int32 transit);
		void _UnsetTransit(BMessage* message);
		bool _AddTokens(BMessage* message, BList& tokens);
		void _RemoveTokens(BMessage* message);
		void _SetToken(BMessage* message, int32 token);

		event_target* _FindListener(const BMessenger& messenger, int32 token,
				int32* _index = NULL);
		bool _AddListener(const BMessenger& messenger, int32 token,
				uint32 eventMask, uint32 options, bool temporary);
		void _RemoveTemporaryListener(event_target* target, int32 index);
		void _RemoveTemporaryListeners();

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
		BList			fLastFocusTokens;
		BList			fFocusTokens;
		bool			fSuspendFocus;

		BMessageFilter*	fMouseFilter;
		BMessageFilter*	fKeyboardFilter;

		BObjectList<event_target> fListeners;

		BLocker			fCursorLock;
		HWInterface*	fHWInterface;
};

#endif	/* EVENT_DISPATCHER_H */
