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
#include "InputManager.h"

#include <Autolock.h>
#include <MessageFilter.h>
#include <View.h>

#include <new>
#include <stdio.h>


//#define TRACE_EVENTS
#ifdef TRACE_EVENTS
#	define ETRACE(x) printf x
#else
#	define ETRACE(x) ;
#endif


/*!
	The differentiation between messenger and token looks odd, but it
	really has a reason as well:
	All events are sent to the window only - it may then use the token or
	token list to identify the specific target view(s).
*/
struct EventDispatcher::event_target {
	BMessenger	messenger;
	int32		token;
	uint32		event_mask;
	uint32		options;
	uint32		temporary_event_mask;
	uint32		temporary_options;
};

static const char* kTokenName = "_token";

static const float kMouseMovedImportance = 0.1f;
static const float kMouseTransitImportance = 1.0f;
static const float kStandardImportance = 0.9f;
static const float kListenerImportance = 0.8f;


EventDispatcher::EventDispatcher()
	: BLocker("event dispatcher"),
	fStream(NULL),
	fThread(-1),
	fCursorThread(-1),
	fHasFocus(false),
	fHasLastFocus(false),
	fTransit(false),
	fSuspendFocus(false),
	fMouseFilter(NULL),
	fKeyboardFilter(NULL),
	fListeners(10, true),
		// the list owns its items
	fCursorLock("cursor loop lock"),
	fHWInterface(NULL)
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

	wait_for_thread(fThread, NULL);
	wait_for_thread(fCursorThread, NULL);

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


void
EventDispatcher::SetFocus(const BMessenger* messenger)
{
	BAutolock _(this);

	if ((messenger == NULL && !fHasFocus)
		|| (messenger != NULL && fFocus == *messenger))
		return;

	fHasLastFocus = fHasFocus;
	fLastFocusTokens.MakeEmpty();

	if (fHasFocus) {
		fLastFocus = fFocus;
		fLastFocusTokens.AddList(&fFocusTokens);
	}

	fHasFocus = messenger != NULL;
	fFocusTokens.MakeEmpty();

	if (fHasFocus) {
		fFocus = *messenger;

		// add all tokens that target this messenger
		for (int32 i = fListeners.CountItems(); i-- > 0;) {
			event_target* target = fListeners.ItemAt(i);

			if (target->messenger == fFocus)
				fFocusTokens.AddItem((void *)target->token);
		}
	}

	fTransit = true;
}


EventDispatcher::event_target*
EventDispatcher::_FindListener(const BMessenger& messenger, int32 token,
	int32* _index)
{
	for (int32 i = fListeners.CountItems(); i-- > 0;) {
		event_target* target = fListeners.ItemAt(i);

		if (target->token == token && target->messenger == messenger) {
			if (_index)
				*_index = i;
			return target;
		}
	}

	return NULL;
}


/*!
	\brief Adds the specified listener or updates its event mask and options
		if already added.

	It follows the BView semantics in that specifiying an event mask of zero
	leaves the event mask untouched and just updates the options.
*/
bool
EventDispatcher::_AddListener(const BMessenger& messenger, int32 token,
	uint32 eventMask, uint32 options, bool temporary)
{
	BAutolock _(this);
	event_target* target = _FindListener(messenger, token);
	if (target != NULL) {
		// we already have this target, update its event mask
		if (temporary) {
			if (eventMask != 0)
				target->temporary_event_mask = eventMask;
			target->temporary_options = options;
		} else {
			if (eventMask != 0)
				target->event_mask = eventMask;
			target->options = options;
		}

		return true;
	}

	if (eventMask == 0)
		return false;

	ETRACE(("events: add listener: token %ld, eventMask = %ld, options = %ld, %s\n",
		token, eventMask, options, temporary ? "temporary" : "permanent"));

	// we need a new target

	target = new (std::nothrow) event_target;
	if (target == NULL)
		return false;

	target->messenger = messenger;
	target->token = token;

	if (temporary) {
		target->event_mask = 0;
		target->options = 0;
		target->temporary_event_mask = eventMask;
		target->temporary_options = options;
		if (options & B_SUSPEND_VIEW_FOCUS)
			fSuspendFocus = true;
	} else {
		target->event_mask = eventMask;
		target->options = options;
		target->temporary_event_mask = 0;
		target->temporary_options = 0;
	}

	bool success = fListeners.AddItem(target);
	if (success) {
		if (fHasFocus && fFocus == messenger) {
			// add this token to the current token list
			ETRACE(("  add token %ld to focus list\n", token));
			fFocusTokens.AddItem((void *)token);
		}
	} else
		delete target;

	return success;
}


void
EventDispatcher::_RemoveTemporaryListener(event_target* target, int32 index)
{
	if (target->event_mask == 0) {
		// this is only a temporary target
		fListeners.RemoveItemAt(index);

		ETRACE(("events: remove temp. listener: token %ld, eventMask = %ld, options = %ld\n",
			target->token, target->temporary_event_mask, target->temporary_options));

		if (fHasFocus && fFocus == target->messenger) {
			// remove this token from the current token list
			ETRACE(("  remove token %ld from focus list\n",
				target->token));
			fFocusTokens.RemoveItem((void *)target->token);
		}

		delete target;
	} else if (target->temporary_event_mask != 0) {
		ETRACE(("events: clear temp. listener: token %ld, eventMask = %ld, options = %ld\n",
			target->token, target->temporary_event_mask, target->temporary_options));

		target->temporary_event_mask = 0;
		target->temporary_options = 0;
	}
}


void
EventDispatcher::_RemoveTemporaryListeners()
{
	for (int32 i = fListeners.CountItems(); i-- > 0;) {
		event_target* target = fListeners.ItemAt(i);

		_RemoveTemporaryListener(target, i);
	}
}


bool
EventDispatcher::AddListener(const BMessenger& messenger, int32 token,
	uint32 eventMask, uint32 options)
{
	options &= B_NO_POINTER_HISTORY;
		// that's currently the only allowed option

	return _AddListener(messenger, token, eventMask, options, false);
}


bool
EventDispatcher::AddTemporaryListener(const BMessenger& messenger,
	int32 token, uint32 eventMask, uint32 options)
{
	return _AddListener(messenger, token, eventMask, options, true);
}


void
EventDispatcher::RemoveListener(const BMessenger& messenger, int32 token)
{
	BAutolock _(this);
	ETRACE(("events: remove listener token %ld\n", token));

	int32 index;
	event_target* target = _FindListener(messenger, token, &index);
	if (target == NULL)
		return;

	if (target->temporary_event_mask != 0) {
		// we still need this event
		target->event_mask = 0;
		target->options = 0;
		return;
	}

	if (fHasFocus && fFocus == target->messenger) {
		// remove this token from the current token list
		ETRACE(("  remove token %ld from focus list\n",
			target->token));
		fFocusTokens.RemoveItem((void *)target->token);
	}

	fListeners.RemoveItemAt(index);
	delete target;
}


void
EventDispatcher::RemoveTemporaryListener(const BMessenger& messenger, int32 token)
{
	BAutolock _(this);

	int32 index;
	event_target* target = _FindListener(messenger, token, &index);
	if (target == NULL)
		return;

	_RemoveTemporaryListener(target, index);
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
EventDispatcher::SetKeyboardFilter(BMessageFilter* filter)
{
	BAutolock _(this);

	if (fKeyboardFilter == filter)
		return;

	delete fKeyboardFilter;
	fKeyboardFilter = filter;
}


void
EventDispatcher::GetMouse(BPoint& where, int32& buttons)
{
	BAutolock _(this);

	where = fLastCursorPosition;
	buttons = fLastButtons;
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

	status_t status = messenger.SendMessage(message, (BHandler*)NULL, 100000);
	if (status != B_OK)
		printf("failed to send message to target: %lx\n", message->what);

	if (status == B_BAD_PORT_ID) {
		// the target port is gone
		return false;
	}

	return true;
}


bool
EventDispatcher::_AddTokens(BMessage* message, BList& tokens)
{
	_RemoveTokens(message);

	int32 count = tokens.CountItems();
	for (int32 i = count; i-- > 0;) {
		int32 token = (int32)tokens.ItemAt(i);
		ETRACE(("  add token %ld\n", token));

		if (message->AddInt32(kTokenName, token) != B_OK)
			count--;
	}

	return count != 0;
}


void
EventDispatcher::_RemoveTokens(BMessage* message)
{
	message->RemoveName(kTokenName);
}


void
EventDispatcher::_SetToken(BMessage* message, int32 token)
{
	if (message->ReplaceInt32(kTokenName, token) != B_OK)
		message->AddInt32(kTokenName, token);
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
		bool pointerEvent = false;
		bool keyboardEvent = false;
		bool addedTransit = false;
		bool addedTokens = false;

		switch (event->what) {
			case B_MOUSE_MOVED:
			{
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
						addedTokens = _AddTokens(event, fLastFocusTokens);
						_SendMessage(fLastFocus, event, kMouseTransitImportance);
						sendToLastFocus = true;

						// we no longer need the last focus messenger
						fHasLastFocus = false;
						fLastFocusTokens.MakeEmpty();
					}

					fTransit = false;
					addedTransit = true;
				}

				BPoint where;
				if (event->FindPoint("where", &where) == B_OK)
					fLastCursorPosition = where;

				// supposed to fall through
			}
			case B_MOUSE_DOWN:
			case B_MOUSE_UP:
				if (fMouseFilter != NULL
					&& fMouseFilter->Filter(event, NULL) == B_SKIP_MESSAGE) {
					// this is a work-around if the wrong B_MOUSE_UP
					// event is filtered out
					if (event->what == B_MOUSE_UP) {
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

				// the "where" field will be filled in by the receiver
				// (it's supposed to be expressed in local window coordinates)
				event->RemoveName("where");
				event->AddPoint("screen_where", fLastCursorPosition);

				pointerEvent = true;

				if (fHasFocus) {
					addedTokens |= _AddTokens(event, fFocusTokens);
					if (addedTokens)
						_SetFeedFocus(event);
					_SendMessage(fFocus, event, event->what == B_MOUSE_MOVED
						? kMouseMovedImportance : kStandardImportance);
				}
				break;

			case B_KEY_DOWN:
			case B_KEY_UP:
			case B_UNMAPPED_KEY_DOWN:
			case B_UNMAPPED_KEY_UP:
			case B_MODIFIERS_CHANGED:
				if (fKeyboardFilter != NULL
					&& fKeyboardFilter->Filter(event, NULL) == B_SKIP_MESSAGE)
					break;

				keyboardEvent = true;

				if (fHasFocus && _AddTokens(event, fFocusTokens)) {
					// if tokens were added, we need to explicetly suspend
					// focus in the event - if not, the event is simply not
					// forwarded to the target
					addedTokens = true;

					if (!fSuspendFocus)
						_SetFeedFocus(event);
				}

				// supposed to fall through

			default:
				if (fHasFocus && (!fSuspendFocus || addedTokens))
					_SendMessage(fFocus, event, kStandardImportance);
				break;
		}

		if (keyboardEvent || pointerEvent) {
			// send the event to the additional listeners

			if (addedTokens) {
				_RemoveTokens(event);
				_UnsetFeedFocus(event);
			}
			if (pointerEvent) {
				// this is added in the RootLayer mouse processing
				// but it's only intended for the focus view
				event->RemoveName("_view_token");
			}

			for (int32 i = fListeners.CountItems(); i-- > 0;) {
				event_target* target = fListeners.ItemAt(i);

				// we already sent the event to the all focus and last focus tokens
				if ((fHasFocus && target->messenger == fFocus)
					|| (sendToLastFocus && target->messenger == fLastFocus))
					continue;

				uint32 effectiveEvents = target->event_mask | target->temporary_event_mask;

				// make sure only those interested listeners get the message
				if ((keyboardEvent && (effectiveEvents & B_KEYBOARD_EVENTS) == 0)
					|| (pointerEvent && (effectiveEvents & B_POINTER_EVENTS) == 0))
					continue;

				_SetToken(event, target->token);
				if (!_SendMessage(target->messenger, event, event->what == B_MOUSE_MOVED
						? kMouseMovedImportance : kListenerImportance)) {
					// the target doesn't seem to exist anymore, let's remove this target
					fListeners.RemoveItemAt(i);
					delete target;
				}
			}

			if (event->what == B_MOUSE_UP) {
				fSuspendFocus = false;
				_RemoveTemporaryListeners();
			}
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
