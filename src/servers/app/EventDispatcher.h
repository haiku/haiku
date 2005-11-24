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

		void GetMouse(BPoint& where, int32& buttons);

		bool HasCursorThread();
		void SetHWInterface(HWInterface* interface);

	private:
		struct event_listener;
		class Target;

		status_t _Run();
		void _Unset();

		bool _SendMessage(BMessenger& messenger, BMessage* message, float importance);

		bool _AddTokens(BMessage* message, Target* target, uint32 eventMask);
		void _RemoveTokens(BMessage* message);
		void _SetToken(BMessage* message, int32 token);
		void _SetFeedFocus(BMessage* message);
		void _UnsetFeedFocus(BMessage* message);

		void _UnsetLastFocus();

		Target* _FindTarget(const BMessenger& messenger, int32* _index = NULL);
		Target* _AddTarget(const BMessenger& messenger);
		void _RemoveTarget(Target* target);

		event_listener* _FindListener(const BMessenger& messenger, int32 token,
				int32* _index = NULL);
		bool _AddListener(const BMessenger& messenger, int32 token,
				uint32 eventMask, uint32 options, bool temporary);
//		void _RemoveTemporaryListener(event_target* target, int32 index);
		void _RemoveTemporaryListeners();

		void _EventLoop();
		void _CursorLoop();

		static status_t _event_looper(void* dispatcher);
		static status_t _cursor_looper(void* dispatcher);

	private:
		EventStream*	fStream;
		thread_id		fThread;
		thread_id		fCursorThread;

		Target*			fFocus;
		Target*			fLastFocus;
		bool			fTransit;
		bool			fSuspendFocus;

		BMessageFilter*	fMouseFilter;
		BMessageFilter*	fKeyboardFilter;

		BObjectList<Target> fTargets;

		BPoint			fLastCursorPosition;
		int32			fLastButtons;

		BLocker			fCursorLock;
		HWInterface*	fHWInterface;
};

#endif	/* EVENT_DISPATCHER_H */
