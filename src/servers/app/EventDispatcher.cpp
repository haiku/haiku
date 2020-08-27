/*
 * Copyright 2005-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "EventDispatcher.h"

#include "BitmapManager.h"
#include "Desktop.h"
#include "EventStream.h"
#include "HWInterface.h"
#include "InputManager.h"
#include "ServerBitmap.h"

#include <MessagePrivate.h>
#include <MessengerPrivate.h>
#include <ServerProtocol.h>
#include <TokenSpace.h>

#include <Autolock.h>
#include <ToolTipManager.h>
#include <View.h>

#include <new>
#include <stdio.h>
#include <string.h>


//#define TRACE_EVENTS
#ifdef TRACE_EVENTS
#	define ETRACE(x) printf x
#else
#	define ETRACE(x) ;
#endif


/*!
	The EventDispatcher is a per Desktop object that handles all input
	events for that desktop.

	The events are processed as needed in the Desktop class (via EventFilters),
	and then forwarded to the actual target of the event, a client window
	(or more correctly, to its EventTarget).
	You cannot set the target of an event directly - the event filters need
	to specify the targets.
	The event loop will make sure that every target and interested listener
	will get the event - it also delivers mouse moved events to the previous
	target once so that this target can then spread the B_EXITED_VIEW transit
	to the local target handler (usually a BView).

	If you look at the event_listener structure below, the differentiation
	between target and token may look odd, but it really has a reason as
	well:
	All events are sent to the preferred window handler only - the window
	may then use the token or token list to identify the specific target
	view(s). This makes it possible to send every event only once, no
	matter how many local target handlers there are.
*/

struct event_listener {
	int32		token;
	uint32		event_mask;
	uint32		options;
	uint32		temporary_event_mask;
	uint32		temporary_options;

	uint32		EffectiveEventMask() const { return event_mask | temporary_event_mask; }
	uint32		EffectiveOptions() const { return options | temporary_options; }
};

static const char* kTokenName = "_token";

static const uint32 kFakeMouseMoved = 'fake';

static const float kMouseMovedImportance = 0.1f;
static const float kMouseTransitImportance = 1.0f;
static const float kStandardImportance = 0.9f;
static const float kListenerImportance = 0.8f;


EventTarget::EventTarget()
	:
	fListeners(2, true)
{
}


EventTarget::~EventTarget()
{
}


void
EventTarget::SetTo(const BMessenger& messenger)
{
	fMessenger = messenger;
}


event_listener*
EventTarget::FindListener(int32 token, int32* _index)
{
	for (int32 i = fListeners.CountItems(); i-- > 0;) {
		event_listener* listener = fListeners.ItemAt(i);

		if (listener->token == token) {
			if (_index)
				*_index = i;
			return listener;
		}
	}

	return NULL;
}


bool
EventTarget::_RemoveTemporaryListener(event_listener* listener, int32 index)
{
	if (listener->event_mask == 0) {
		// this is only a temporary target
		ETRACE(("events: remove temp. listener: token %ld, eventMask = %ld, options = %ld\n",
			listener->token, listener->temporary_event_mask, listener->temporary_options));

		fListeners.RemoveItemAt(index);
		delete listener;
		return true;
	}

	if (listener->temporary_event_mask != 0) {
		ETRACE(("events: clear temp. listener: token %ld, eventMask = %ld, "
				"options = %ld\n",
			listener->token, listener->temporary_event_mask,
			listener->temporary_options));

		listener->temporary_event_mask = 0;
		listener->temporary_options = 0;
	}

	return false;
}


void
EventTarget::RemoveTemporaryListeners()
{
	for (int32 index = CountListeners(); index-- > 0;) {
		event_listener* listener = ListenerAt(index);

		_RemoveTemporaryListener(listener, index);
	}
}


bool
EventTarget::RemoveTemporaryListener(int32 token)
{
	int32 index;
	event_listener* listener = FindListener(token, &index);
	if (listener == NULL)
		return false;

	return _RemoveTemporaryListener(listener, index);
}


bool
EventTarget::RemoveListener(int32 token)
{
	int32 index;
	event_listener* listener = FindListener(token, &index);
	if (listener == NULL)
		return false;

	if (listener->temporary_event_mask != 0) {
		// we still need this event
		listener->event_mask = 0;
		listener->options = 0;
		return false;
	}

	fListeners.RemoveItemAt(index);
	delete listener;
	return true;
}


bool
EventTarget::AddListener(int32 token, uint32 eventMask,
	uint32 options, bool temporary)
{
	event_listener* listener = new (std::nothrow) event_listener;
	if (listener == NULL)
		return false;

	listener->token = token;

	if (temporary) {
		listener->event_mask = 0;
		listener->options = 0;
		listener->temporary_event_mask = eventMask;
		listener->temporary_options = options;
	} else {
		listener->event_mask = eventMask;
		listener->options = options;
		listener->temporary_event_mask = 0;
		listener->temporary_options = 0;
	}

	bool success = fListeners.AddItem(listener);
	if (!success)
		delete listener;

	return success;
}


//	#pragma mark -


void
EventFilter::RemoveTarget(EventTarget* target)
{
}


//	#pragma mark -


EventDispatcher::EventDispatcher()
	:
	BLocker("event dispatcher"),
	fStream(NULL),
	fThread(-1),
	fCursorThread(-1),
	fPreviousMouseTarget(NULL),
	fFocus(NULL),
	fSuspendFocus(false),
	fMouseFilter(NULL),
	fKeyboardFilter(NULL),
	fTargets(10),
	fNextLatestMouseMoved(NULL),
	fLastButtons(0),
	fLastUpdate(system_time()),
	fDraggingMessage(false),
	fCursorLock("cursor loop lock"),
	fHWInterface(NULL),
	fDesktop(NULL)
{
}


EventDispatcher::~EventDispatcher()
{
	_Unset();
}


status_t
EventDispatcher::SetTo(EventStream* stream)
{
	ETRACE(("event dispatcher: stream = %p\n", stream));

	_Unset();

	if (stream == NULL)
		return B_OK;

	fStream = stream;
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
	if (fStream == NULL)
		return;

	fStream->SendQuit();

	status_t status;
	wait_for_thread(fThread, &status);
	wait_for_thread(fCursorThread, &status);

	fThread = fCursorThread = -1;

	gInputManager->PutStream(fStream);
	fStream = NULL;
}


status_t
EventDispatcher::_Run()
{
	fThread = spawn_thread(_event_looper, "event loop",
		B_REAL_TIME_DISPLAY_PRIORITY - 10, this);
	if (fThread < B_OK)
		return fThread;

	if (fStream->SupportsCursorThread()) {
		ETRACE(("event stream supports cursor thread!\n"));

		fCursorThread = spawn_thread(_cursor_looper, "cursor loop",
			B_REAL_TIME_DISPLAY_PRIORITY - 5, this);
		if (resume_thread(fCursorThread) != B_OK) {
			kill_thread(fCursorThread);
			fCursorThread = -1;
		}
	}

	return resume_thread(fThread);
}


/*!
	\brief Removes any reference to the target, but doesn't delete it.
*/
void
EventDispatcher::RemoveTarget(EventTarget& target)
{
	BAutolock _(this);

	if (fFocus == &target)
		fFocus = NULL;
	if (fPreviousMouseTarget == &target)
		fPreviousMouseTarget = NULL;

	if (fKeyboardFilter.Get() != NULL)
		fKeyboardFilter->RemoveTarget(&target);
	if (fMouseFilter.Get() != NULL)
		fMouseFilter->RemoveTarget(&target);

	fTargets.RemoveItem(&target);
}


/*!
	\brief Adds the specified listener or updates its event mask and options
		if already added.

	It follows the BView semantics in that specifiying an event mask of zero
	leaves the event mask untouched and just updates the options.
*/
bool
EventDispatcher::_AddListener(EventTarget& target, int32 token,
	uint32 eventMask, uint32 options, bool temporary)
{
	BAutolock _(this);

	if (temporary && fLastButtons == 0) {
		// only allow to add temporary listeners in case a buttons is pressed
		return false;
	}

	if (!fTargets.HasItem(&target))
		fTargets.AddItem(&target);

	event_listener* listener = target.FindListener(token);
	if (listener != NULL) {
		// we already have this target, update its event mask
		if (temporary) {
			if (eventMask != 0)
				listener->temporary_event_mask = eventMask;
			listener->temporary_options = options;
		} else {
			if (eventMask != 0)
				listener->event_mask = eventMask;
			listener->options = options;
		}

		return true;
	}

	if (eventMask == 0)
		return false;

	ETRACE(("events: add listener: token %ld, eventMask = %ld, options = %ld,"
			"%s\n",
		token, eventMask, options, temporary ? "temporary" : "permanent"));

	// we need a new target

	bool success = target.AddListener(token, eventMask, options, temporary);
	if (!success) {
		if (target.IsEmpty())
			fTargets.RemoveItem(&target);
	} else {
		if (options & B_SUSPEND_VIEW_FOCUS)
			fSuspendFocus = true;
	}

	return success;
}


void
EventDispatcher::_RemoveTemporaryListeners()
{
	for (int32 i = fTargets.CountItems(); i-- > 0;) {
		EventTarget* target = fTargets.ItemAt(i);

		target->RemoveTemporaryListeners();
	}
}


bool
EventDispatcher::AddListener(EventTarget& target, int32 token,
	uint32 eventMask, uint32 options)
{
	options &= B_NO_POINTER_HISTORY;
		// that's currently the only allowed option

	return _AddListener(target, token, eventMask, options, false);
}


bool
EventDispatcher::AddTemporaryListener(EventTarget& target,
	int32 token, uint32 eventMask, uint32 options)
{
	return _AddListener(target, token, eventMask, options, true);
}


void
EventDispatcher::RemoveListener(EventTarget& target, int32 token)
{
	BAutolock _(this);
	ETRACE(("events: remove listener token %ld\n", token));

	if (target.RemoveListener(token) && target.IsEmpty())
		fTargets.RemoveItem(&target);
}


void
EventDispatcher::RemoveTemporaryListener(EventTarget& target, int32 token)
{
	BAutolock _(this);
	ETRACE(("events: remove temporary listener token %ld\n", token));

	if (target.RemoveTemporaryListener(token) && target.IsEmpty())
		fTargets.RemoveItem(&target);
}


void
EventDispatcher::SetMouseFilter(EventFilter* filter)
{
	BAutolock _(this);

	if (fMouseFilter.Get() == filter)
		return;

	fMouseFilter.SetTo(filter);
}


void
EventDispatcher::SetKeyboardFilter(EventFilter* filter)
{
	BAutolock _(this);

	if (fKeyboardFilter.Get() == filter)
		return;

	fKeyboardFilter.SetTo(filter);
}


void
EventDispatcher::GetMouse(BPoint& where, int32& buttons)
{
	BAutolock _(this);

	where = fLastCursorPosition;
	buttons = fLastButtons;
}


void
EventDispatcher::SendFakeMouseMoved(EventTarget& target, int32 viewToken)
{
	if (fStream == NULL)
		return;

	BMessage* fakeMove = new BMessage(kFakeMouseMoved);
	if (fakeMove == NULL)
		return;

	fakeMove->AddMessenger("target", target.Messenger());
	fakeMove->AddInt32("view_token", viewToken);

	fStream->InsertEvent(fakeMove);
}


void
EventDispatcher::_SendFakeMouseMoved(BMessage* message)
{
	BMessenger target;
	int32 viewToken;
	if (message->FindInt32("view_token", &viewToken) != B_OK
		|| message->FindMessenger("target", &target) != B_OK)
		return;

	if (fDesktop == NULL)
		return;

	// Check if the target is still valid
	::EventTarget* eventTarget = NULL;

	fDesktop->LockSingleWindow();

	if (target.IsValid())
		eventTarget = fDesktop->FindTarget(target);

	fDesktop->UnlockSingleWindow();

	if (eventTarget == NULL)
		return;

	BMessage moved(B_MOUSE_MOVED);
	moved.AddPoint("screen_where", fLastCursorPosition);
	moved.AddInt32("buttons", fLastButtons);

	if (fDraggingMessage)
		moved.AddMessage("be:drag_message", &fDragMessage);

	if (fPreviousMouseTarget != NULL
		&& fPreviousMouseTarget->Messenger() != target) {
		_AddTokens(&moved, fPreviousMouseTarget, B_POINTER_EVENTS);
		_SendMessage(fPreviousMouseTarget->Messenger(), &moved,
			kMouseTransitImportance);

		_RemoveTokens(&moved);
	}

	moved.AddInt32("_view_token", viewToken);
		// this only belongs to the new target

	moved.AddBool("be:transit_only", true);
		// let the view know this what not user generated

	_SendMessage(target, &moved, kMouseTransitImportance);

	fPreviousMouseTarget = eventTarget;
}


bigtime_t
EventDispatcher::IdleTime()
{
	BAutolock _(this);
	return system_time() - fLastUpdate;
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

	// adopt the cursor position of the new HW interface
	if (interface != NULL)
		fLastCursorPosition = interface->CursorPosition();
}


void
EventDispatcher::SetDragMessage(BMessage& message,
	ServerBitmap* bitmap, const BPoint& offsetFromCursor)
{
	ETRACE(("EventDispatcher::SetDragMessage()\n"));

	BAutolock _(this);

	if (fLastButtons == 0) {
		// mouse buttons has already been released or was never pressed
		return;
	}

	fHWInterface->SetDragBitmap(bitmap, offsetFromCursor);

	fDragMessage = message;
	fDraggingMessage = true;
	fDragOffset = offsetFromCursor;
}


void
EventDispatcher::SetDesktop(Desktop* desktop)
{
	fDesktop = desktop;
}


//	#pragma mark - Message methods


/*!
	\brief Sends \a message to the provided \a messenger.

	TODO: the following feature is not yet implemented:
	If the message could not be delivered immediately, it is included
	in a waiting message queue with a fixed length - the least important
	messages are removed first when that gets full.

	Returns "false" if the target port does not exist anymore, "true"
	if it doesn't.
*/
bool
EventDispatcher::_SendMessage(BMessenger& messenger, BMessage* message,
	float importance)
{
	// TODO: add failed messages to a queue, and start dropping them by importance
	//	(and use the same mechanism in ServerWindow::SendMessageToClient())

	status_t status = messenger.SendMessage(message, (BHandler*)NULL, 0);
	if (status != B_OK) {
		printf("EventDispatcher: failed to send message '%.4s' to target: %s\n",
			(char*)&message->what, strerror(status));
	}

	if (status == B_BAD_PORT_ID) {
		// the target port is gone
		return false;
	}

	return true;
}


bool
EventDispatcher::_AddTokens(BMessage* message, EventTarget* target,
	uint32 eventMask, BMessage* nextMouseMoved, int32* _viewToken)
{
	_RemoveTokens(message);

	int32 count = target->CountListeners();
	int32 added = 0;

	for (int32 i = 0; i < count; i++) {
		event_listener* listener = target->ListenerAt(i);
		if ((listener->EffectiveEventMask() & eventMask) == 0)
			continue;

		if (nextMouseMoved != NULL && message->what == B_MOUSE_MOVED
			&& (listener->EffectiveOptions() & B_NO_POINTER_HISTORY) != 0
			&& message != nextMouseMoved
			&& _viewToken != NULL) {
			if (listener->token == *_viewToken) {
				// focus view doesn't want to get pointer history
				*_viewToken = B_NULL_TOKEN;
			}
			continue;
		}

		ETRACE(("  add token %ld\n", listener->token));

		if (message->AddInt32(kTokenName, listener->token) == B_OK)
			added++;
	}

	return added != 0;
}


void
EventDispatcher::_RemoveTokens(BMessage* message)
{
	message->RemoveName(kTokenName);
}


void
EventDispatcher::_SetFeedFocus(BMessage* message)
{
	if (message->ReplaceBool("_feed_focus", true) != B_OK)
		message->AddBool("_feed_focus", true);
}


void
EventDispatcher::_UnsetFeedFocus(BMessage* message)
{
	message->RemoveName("_feed_focus");
}


void
EventDispatcher::_DeliverDragMessage()
{
	ETRACE(("EventDispatcher::_DeliverDragMessage()\n"));

	if (fDraggingMessage && fPreviousMouseTarget != NULL) {
		BMessage::Private(fDragMessage).SetWasDropped(true);
		fDragMessage.RemoveName("_original_what");
		fDragMessage.AddInt32("_original_what", fDragMessage.what);
		fDragMessage.AddPoint("_drop_point_", fLastCursorPosition);
		fDragMessage.AddPoint("_drop_offset_", fDragOffset);
		fDragMessage.what = _MESSAGE_DROPPED_;

		_SendMessage(fPreviousMouseTarget->Messenger(),
			&fDragMessage, 100.0);
	}

	fDragMessage.MakeEmpty();
	fDragMessage.what = 0;
	fDraggingMessage = false;

	fHWInterface->SetDragBitmap(NULL, B_ORIGIN);
}


//	#pragma mark - Event loops


void
EventDispatcher::_EventLoop()
{
	BMessage* event;
	while (fStream->GetNextEvent(&event)) {
		BAutolock _(this);
		fLastUpdate = system_time();

		EventTarget* current = NULL;
		EventTarget* previous = NULL;
		bool pointerEvent = false;
		bool keyboardEvent = false;
		bool addedTokens = false;

		switch (event->what) {
			case kFakeMouseMoved:
				_SendFakeMouseMoved(event);
				break;
			case B_MOUSE_MOVED:
			{
				BPoint where;
				if (event->FindPoint("where", &where) == B_OK)
					fLastCursorPosition = where;

				if (fDraggingMessage)
					event->AddMessage("be:drag_message", &fDragMessage);

				if (!HasCursorThread()) {
					// There is no cursor thread, we need to move the cursor
					// ourselves
					BAutolock _(fCursorLock);

					if (fHWInterface != NULL) {
						fHWInterface->MoveCursorTo(fLastCursorPosition.x,
							fLastCursorPosition.y);
					}
				}

				// This is for B_NO_POINTER_HISTORY - we always want the
				// latest mouse moved event in the queue only
				if (fNextLatestMouseMoved == NULL)
					fNextLatestMouseMoved = fStream->PeekLatestMouseMoved();
				else if (fNextLatestMouseMoved != event) {
					// Drop older mouse moved messages if the server is lagging
					// too much (if the message is older than 100 msecs)
					bigtime_t eventTime;
					if (event->FindInt64("when", &eventTime) == B_OK) {
						if (system_time() - eventTime > 100000)
							break;
					}
				}

				// supposed to fall through
			}
			case B_MOUSE_DOWN:
			case B_MOUSE_UP:
			case B_MOUSE_IDLE:
			{
#ifdef TRACE_EVENTS
				if (event->what != B_MOUSE_MOVED)
					printf("mouse up/down event, previous target = %p\n", fPreviousMouseTarget);
#endif
				pointerEvent = true;

				if (fMouseFilter.Get() == NULL)
					break;

				EventTarget* mouseTarget = fPreviousMouseTarget;
				int32 viewToken = B_NULL_TOKEN;
				if (fMouseFilter->Filter(event, &mouseTarget, &viewToken,
						fNextLatestMouseMoved) == B_SKIP_MESSAGE) {
					// this is a work-around if the wrong B_MOUSE_UP
					// event is filtered out
					if (event->what == B_MOUSE_UP
						&& event->FindInt32("buttons") == 0) {
						fSuspendFocus = false;
						_RemoveTemporaryListeners();
					}
					break;
				}

				int32 buttons;
				if (event->FindInt32("buttons", &buttons) == B_OK)
					fLastButtons = buttons;
				else
					fLastButtons = 0;

				// The "where" field will be filled in by the receiver
				// (it's supposed to be expressed in local window coordinates)
				event->RemoveName("where");
				event->AddPoint("screen_where", fLastCursorPosition);

				if (event->what == B_MOUSE_MOVED
					&& fPreviousMouseTarget != NULL
					&& mouseTarget != fPreviousMouseTarget) {
					// Target has changed, we need to notify the previous target
					// that the mouse has exited its views
					addedTokens = _AddTokens(event, fPreviousMouseTarget,
						B_POINTER_EVENTS);
					if (addedTokens)
						_SetFeedFocus(event);

					_SendMessage(fPreviousMouseTarget->Messenger(), event,
						kMouseTransitImportance);
					previous = fPreviousMouseTarget;
				}

				current = fPreviousMouseTarget = mouseTarget;

				if (current != NULL) {
					int32 focusView = viewToken;
					addedTokens |= _AddTokens(event, current, B_POINTER_EVENTS,
						fNextLatestMouseMoved, &focusView);

					bool noPointerHistoryFocus = focusView != viewToken;

					if (viewToken != B_NULL_TOKEN)
						event->AddInt32("_view_token", viewToken);

					if (addedTokens && !noPointerHistoryFocus)
						_SetFeedFocus(event);
					else if (noPointerHistoryFocus) {
						// No tokens were added or the focus shouldn't get a
						// mouse moved
						break;
					}

					_SendMessage(current->Messenger(), event,
						event->what == B_MOUSE_MOVED
							? kMouseMovedImportance : kStandardImportance);
				}
				break;
			}

			case B_KEY_DOWN:
			case B_KEY_UP:
			case B_UNMAPPED_KEY_DOWN:
			case B_UNMAPPED_KEY_UP:
			case B_MODIFIERS_CHANGED:
			case B_INPUT_METHOD_EVENT:
				ETRACE(("key event, focus = %p\n", fFocus));

				if (fKeyboardFilter.Get() != NULL
					&& fKeyboardFilter->Filter(event, &fFocus)
						== B_SKIP_MESSAGE) {
					break;
				}

				keyboardEvent = true;

				if (fFocus != NULL && _AddTokens(event, fFocus,
						B_KEYBOARD_EVENTS)) {
					// if tokens were added, we need to explicetly suspend
					// focus in the event - if not, the event is simply not
					// forwarded to the target
					addedTokens = true;

					if (!fSuspendFocus)
						_SetFeedFocus(event);
				}

				// supposed to fall through

			default:
				// TODO: the keyboard filter sets the focus - ie. no other
				//	focus messages that go through the event dispatcher can
				//	go through.
				if (event->what == B_MOUSE_WHEEL_CHANGED)
					current = fPreviousMouseTarget;
				else
					current = fFocus;

				if (current != NULL && (!fSuspendFocus || addedTokens)) {
					_SendMessage(current->Messenger(), event,
						kStandardImportance);
				}
				break;
		}

		if (keyboardEvent || pointerEvent) {
			// send the event to the additional listeners

			if (addedTokens) {
				_RemoveTokens(event);
				_UnsetFeedFocus(event);
			}
			if (pointerEvent) {
				// this is added in the Desktop mouse processing
				// but it's only intended for the focus view
				event->RemoveName("_view_token");
			}

			for (int32 i = fTargets.CountItems(); i-- > 0;) {
				EventTarget* target = fTargets.ItemAt(i);

				// We already sent the event to the all focus and last focus
				// tokens
				if (current == target || previous == target)
					continue;

				// Don't send the message if there are no tokens for this event
				if (!_AddTokens(event, target,
						keyboardEvent ? B_KEYBOARD_EVENTS : B_POINTER_EVENTS,
						event->what == B_MOUSE_MOVED
							? fNextLatestMouseMoved : NULL))
					continue;

				if (!_SendMessage(target->Messenger(), event,
						event->what == B_MOUSE_MOVED
							? kMouseMovedImportance : kListenerImportance)) {
					// the target doesn't seem to exist anymore, let's remove it
					fTargets.RemoveItemAt(i);
				}
			}

			if (event->what == B_MOUSE_UP && fLastButtons == 0) {
				// no buttons are pressed anymore
				fSuspendFocus = false;
				_RemoveTemporaryListeners();
				if (fDraggingMessage)
					_DeliverDragMessage();
			}
		}

		if (fNextLatestMouseMoved == event)
			fNextLatestMouseMoved = NULL;
		delete event;
	}

	// The loop quit, therefore no more events are coming from the input
	// server, it must have died. Unset ourselves and notify the desktop.
	fThread = -1;
		// Needed to avoid problems with wait_for_thread in _Unset()
	_Unset();

	if (fDesktop)
		fDesktop->PostMessage(AS_EVENT_STREAM_CLOSED);
}


void
EventDispatcher::_CursorLoop()
{
	BPoint where;
	const bigtime_t toolTipDelay = BToolTipManager::Manager()->ShowDelay();
	bool mouseIdleSent = true;
	status_t status = B_OK;

	while (status != B_ERROR) {
		const bigtime_t timeout = mouseIdleSent ?
			B_INFINITE_TIMEOUT : toolTipDelay;
		status = fStream->GetNextCursorPosition(where, timeout);

		if (status == B_OK) {
			mouseIdleSent = false;
			BAutolock _(fCursorLock);

			if (fHWInterface != NULL)
				fHWInterface->MoveCursorTo(where.x, where.y);
		} else if (status == B_TIMED_OUT) {
			mouseIdleSent = true;
			BMessage* mouseIdle = new BMessage(B_MOUSE_IDLE);
			mouseIdle->AddPoint("screen_where", fLastCursorPosition);
			fStream->InsertEvent(mouseIdle);
		}
	}

	fCursorThread = -1;
}


/*static*/
status_t
EventDispatcher::_event_looper(void* _dispatcher)
{
	EventDispatcher* dispatcher = (EventDispatcher*)_dispatcher;

	ETRACE(("Start event loop\n"));
	dispatcher->_EventLoop();
	return B_OK;
}


/*static*/
status_t
EventDispatcher::_cursor_looper(void* _dispatcher)
{
	EventDispatcher* dispatcher = (EventDispatcher*)_dispatcher;

	ETRACE(("Start cursor loop\n"));
	dispatcher->_CursorLoop();
	return B_OK;
}
