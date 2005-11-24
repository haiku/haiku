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


// TODO: the Target class could be made public, and ServerWindow could inherit
//	from it - this would especially be nice if we didn't have to lock the root
//	layer when messing with ServerWindow children (because then, RootLayer's
//	mouse filter could almost be moved completely to the window).

/*!
	The differentiation between messenger and token looks odd, but it
	really has a reason as well:
	All events are sent to the preferred window handler only - the window
	may then use the token or token list to identify the specific target
	view(s).
*/

class EventDispatcher::Target {
	public:
		Target(const BMessenger& messenger);
		~Target();

		BMessenger& Messenger() { return fMessenger; }

		event_listener* FindListener(int32 token, int32* _index = NULL);
		bool AddListener(int32 token, uint32 eventMask, uint32 options,
				bool temporary);
		void RemoveListener(event_listener* listener, bool temporary);

		bool RemoveListener(int32 token);
		bool RemoveTemporaryListener(int32 token);
		void RemoveTemporaryListeners();

		bool IsNeeded() const { return fFocusOrLastFocus || !fListeners.IsEmpty(); }

		void SetFocusOrLastFocus(bool focus) { fFocusOrLastFocus = focus; }
		bool FocusOrLastFocus() const { return fFocusOrLastFocus; }

		int32 CountListeners() const { return fListeners.CountItems(); }
		event_listener* ListenerAt(int32 index) const { return fListeners.ItemAt(index); }

	private:
		bool _RemoveTemporaryListener(event_listener* listener, int32 index);

		BObjectList<event_listener> fListeners;
		BMessenger	fMessenger;
		bool		fFocusOrLastFocus;

};

struct EventDispatcher::event_listener {
	int32		token;
	uint32		event_mask;
	uint32		options;
	uint32		temporary_event_mask;
	uint32		temporary_options;

	uint32		EffectiveEventMask() const { return event_mask | temporary_event_mask; }
};

static const char* kTokenName = "_token";

static const float kMouseMovedImportance = 0.1f;
static const float kMouseTransitImportance = 1.0f;
static const float kStandardImportance = 0.9f;
static const float kListenerImportance = 0.8f;


EventDispatcher::Target::Target(const BMessenger& messenger)
	:
	fListeners(2, true),
	fMessenger(messenger),
	fFocusOrLastFocus(false)
{
}


EventDispatcher::Target::~Target()
{
}


EventDispatcher::event_listener*
EventDispatcher::Target::FindListener(int32 token, int32* _index)
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
EventDispatcher::Target::_RemoveTemporaryListener(event_listener* listener, int32 index)
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
		ETRACE(("events: clear temp. listener: token %ld, eventMask = %ld, options = %ld\n",
			listener->token, listener->temporary_event_mask, listener->temporary_options));

		listener->temporary_event_mask = 0;
		listener->temporary_options = 0;
	}

	return false;
}


void
EventDispatcher::Target::RemoveTemporaryListeners()
{
	for (int32 index = CountListeners(); index-- > 0;) {
		event_listener* listener = ListenerAt(index);

		_RemoveTemporaryListener(listener, index);
	}
}


bool
EventDispatcher::Target::RemoveTemporaryListener(int32 token)
{
	int32 index;
	event_listener* listener = FindListener(token, &index);
	if (listener == NULL)
		return false;

	return _RemoveTemporaryListener(listener, index);
}


bool
EventDispatcher::Target::RemoveListener(int32 token)
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
EventDispatcher::Target::AddListener(int32 token, uint32 eventMask,
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


EventDispatcher::EventDispatcher()
	: BLocker("event dispatcher"),
	fStream(NULL),
	fThread(-1),
	fCursorThread(-1),
	fFocus(NULL),
	fLastFocus(NULL),
	fTransit(false),
	fSuspendFocus(false),
	fMouseFilter(NULL),
	fKeyboardFilter(NULL),
	fTargets(10, true),
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
EventDispatcher::_UnsetLastFocus()
{
	if (fLastFocus == NULL)
		return;

	fLastFocus->SetFocusOrLastFocus(false);
	if (!fLastFocus->IsNeeded())
		_RemoveTarget(fLastFocus);

	fLastFocus = NULL;
}


void
EventDispatcher::SetFocus(const BMessenger* messenger)
{
	BAutolock _(this);
	ETRACE(("EventDispatcher::SetFocus(messenger = %p)\n", messenger));

	if ((messenger == NULL && fFocus == NULL)
		|| (messenger != NULL && fFocus != NULL && fFocus->Messenger() == *messenger))
		return;

	// update last focus

	if (fLastFocus != NULL)
		_UnsetLastFocus();

	fLastFocus = fFocus;

	// update focus

	if (messenger != NULL) {
		fFocus = _FindTarget(*messenger);
		if (fFocus == NULL) {
			// we need a new target for this focus
			fFocus = _AddTarget(*messenger);
			if (fFocus == NULL)
				printf("EventDispatcher: could not set focus!\n");
		}
		if (fFocus != NULL)
			fFocus->SetFocusOrLastFocus(true);
	} else
		fFocus = NULL;

	fTransit = true;
}


EventDispatcher::Target*
EventDispatcher::_FindTarget(const BMessenger& messenger, int32* _index)
{
	for (int32 i = fTargets.CountItems(); i-- > 0;) {
		Target* target = fTargets.ItemAt(i);

		if (target->Messenger() == messenger) {
			if (_index)
				*_index = i;
			return target;
		}
	}

	return NULL;
}


EventDispatcher::Target*
EventDispatcher::_AddTarget(const BMessenger& messenger)
{
	Target* target = new (std::nothrow) Target(messenger);
	if (target == NULL) {
		printf("EventDispatcher: could not create new target!\n");
		return NULL;
	}

	bool success = fTargets.AddItem(target);
	if (!success) {
		printf("EventDispatcher: could not add new target!\n");
		delete target;
		return NULL;
	}

	return target;
}


void
EventDispatcher::_RemoveTarget(Target* target)
{
	// the list owns its items and will delete the target itself
	fTargets.RemoveItem(target);
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
	Target* target = _FindTarget(messenger);
	if (target == NULL) {
		// we need a new target for this messenger
		target = _AddTarget(messenger);
		if (target == NULL)
			return false;
	}

	event_listener* listener = target->FindListener(token);
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

	ETRACE(("events: add listener: token %ld, eventMask = %ld, options = %ld, %s\n",
		token, eventMask, options, temporary ? "temporary" : "permanent"));

	// we need a new target

	bool success = target->AddListener(token, eventMask, options, temporary);
	if (!success) {
		if (!target->IsNeeded())
			_RemoveTarget(target);
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
		Target* target = fTargets.ItemAt(i);

		target->RemoveTemporaryListeners();
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
	Target* target = _FindTarget(messenger, &index);
	if (target == NULL)
		return;

	if (target->RemoveListener(token) && !target->IsNeeded()) {
		fTargets.RemoveItemAt(index);
		delete target;
	}
}


void
EventDispatcher::RemoveTemporaryListener(const BMessenger& messenger, int32 token)
{
	BAutolock _(this);

	int32 index;
	Target* target = _FindTarget(messenger, &index);
	if (target == NULL)
		return;

	if (target->RemoveTemporaryListener(token) && !target->IsNeeded()) {
		fTargets.RemoveItemAt(index);
		delete target;
	}
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
EventDispatcher::_AddTokens(BMessage* message, Target* target, uint32 eventMask)
{
	_RemoveTokens(message);

	int32 count = target->CountListeners();
	for (int32 i = count; i-- > 0;) {
		event_listener* listener = target->ListenerAt(i);
		if ((listener->EffectiveEventMask() & eventMask) == 0) {
			count--;
			continue;
		}

		ETRACE(("  add token %ld\n", listener->token));

		if (message->AddInt32(kTokenName, listener->token) != B_OK)
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


//	#pragma mark - Event loops


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
					if (fLastFocus != NULL) {
						addedTokens = _AddTokens(event, fLastFocus, B_POINTER_EVENTS);
						_SendMessage(fLastFocus->Messenger(), event,
							kMouseTransitImportance);
						sendToLastFocus = true;

						// we no longer need the last focus messenger
						_UnsetLastFocus();
					}

					fTransit = false;
				}

				BPoint where;
				if (event->FindPoint("where", &where) == B_OK)
					fLastCursorPosition = where;

				// supposed to fall through
			}
			case B_MOUSE_DOWN:
			case B_MOUSE_UP:
#ifdef TRACE_EVENTS
				if (event->what != B_MOUSE_MOVED)
					printf("mouse up/down event, focus = %p\n", fFocus);
#endif
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

				if (fFocus != NULL) {
					addedTokens |= _AddTokens(event, fFocus, B_POINTER_EVENTS);
					if (addedTokens)
						_SetFeedFocus(event);
					_SendMessage(fFocus->Messenger(), event, event->what == B_MOUSE_MOVED
						? kMouseMovedImportance : kStandardImportance);
				}
				break;

			case B_KEY_DOWN:
			case B_KEY_UP:
			case B_UNMAPPED_KEY_DOWN:
			case B_UNMAPPED_KEY_UP:
			case B_MODIFIERS_CHANGED:
				ETRACE(("key event, focus = %p\n", fFocus));

				if (fKeyboardFilter != NULL
					&& fKeyboardFilter->Filter(event, NULL) == B_SKIP_MESSAGE)
					break;

				keyboardEvent = true;

				if (fFocus != NULL && _AddTokens(event, fFocus, B_KEYBOARD_EVENTS)) {
					// if tokens were added, we need to explicetly suspend
					// focus in the event - if not, the event is simply not
					// forwarded to the target
					addedTokens = true;

					if (!fSuspendFocus)
						_SetFeedFocus(event);
				}

				// supposed to fall through

			default:
				if (fFocus != NULL && (!fSuspendFocus || addedTokens))
					_SendMessage(fFocus->Messenger(), event, kStandardImportance);
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

			for (int32 i = fTargets.CountItems(); i-- > 0;) {
				Target* target = fTargets.ItemAt(i);

				// we already sent the event to the all focus and last focus tokens
				if (fFocus == target || (sendToLastFocus && fLastFocus == target))
					continue;

				// don't send the message if there are no tokens for this event
				if (!_AddTokens(event, target,
						keyboardEvent ? B_KEYBOARD_EVENTS : B_POINTER_EVENTS))
					continue;

				if (!_SendMessage(target->Messenger(), event, event->what == B_MOUSE_MOVED
						? kMouseMovedImportance : kListenerImportance)) {
					// the target doesn't seem to exist anymore, let's remove it
					fTargets.RemoveItemAt(i);
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
