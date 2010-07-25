/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "StateView.h"

#include <new>

#include <Message.h>
#include <MessageFilter.h>
#include <TextView.h>
#include <Window.h>

#include "Command.h"
#include "CommandStack.h"
// TODO: hack - somehow figure out of catching
// key events for a given control is ok
#include "GradientControl.h"
#include "ListViews.h"
//
#include "RWLocker.h"

using std::nothrow;

class EventFilter : public BMessageFilter {
 public:
	EventFilter(StateView* target)
		: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
		  fTarget(target)
		{
		}
	virtual	~EventFilter()
		{
		}
	virtual	filter_result	Filter(BMessage* message, BHandler** target)
		{
			filter_result result = B_DISPATCH_MESSAGE;
			switch (message->what) {
				case B_KEY_DOWN: {
if (dynamic_cast<BTextView*>(*target))
	break;
if (dynamic_cast<SimpleListView*>(*target))
	break;
if (dynamic_cast<GradientControl*>(*target))
	break;
					uint32 key;
					uint32 modifiers;
					if (message->FindInt32("raw_char", (int32*)&key) >= B_OK
						&& message->FindInt32("modifiers", (int32*)&modifiers) >= B_OK)
						if (fTarget->HandleKeyDown(key, modifiers))
							result = B_SKIP_MESSAGE;
					break;
				}
				case B_KEY_UP: {
if (dynamic_cast<BTextView*>(*target))
	break;
if (dynamic_cast<SimpleListView*>(*target))
	break;
if (dynamic_cast<GradientControl*>(*target))
	break;
					uint32 key;
					uint32 modifiers;
					if (message->FindInt32("raw_char", (int32*)&key) >= B_OK
						&& message->FindInt32("modifiers", (int32*)&modifiers) >= B_OK)
						if (fTarget->HandleKeyUp(key, modifiers))
							result = B_SKIP_MESSAGE;
					break;

				}
				case B_MODIFIERS_CHANGED:
					*target = fTarget;
					break;

				case B_MOUSE_WHEEL_CHANGED: {
					float x;
					float y;
					if (message->FindFloat("be:wheel_delta_x", &x) >= B_OK
						&& message->FindFloat("be:wheel_delta_y", &y) >= B_OK) {
						if (fTarget->MouseWheelChanged(
								fTarget->MouseInfo()->position, x, y))
							result = B_SKIP_MESSAGE;
					}
					break;
				}
				default:
					break;
			}
			return result;
		}
 private:
 	StateView*		fTarget;
};

// #pragma mark -

// constructor
StateView::StateView(BRect frame, const char* name,
					 uint32 resizingMode, uint32 flags)
	: BView(frame, name, resizingMode, flags),
	  fCurrentState(NULL),
	  fDropAnticipatingState(NULL),

	  fMouseInfo(),

	  fCommandStack(NULL),
	  fLocker(NULL),

	  fEventFilter(NULL),
	  fCatchAllEvents(false),

	  fUpdateTarget(NULL),
	  fUpdateCommand(0)
{
}

// destructor
StateView::~StateView()
{
	delete fEventFilter;
}

// #pragma mark -

// AttachedToWindow
void
StateView::AttachedToWindow()
{
	_InstallEventFilter();

	BView::AttachedToWindow();
}

// DetachedFromWindow
void
StateView::DetachedFromWindow()
{
	_RemoveEventFilter();

	BView::DetachedFromWindow();
}

// Draw
void
StateView::Draw(BRect updateRect)
{
	Draw(this, updateRect);
}

// MessageReceived
void
StateView::MessageReceived(BMessage* message)
{
	// let the state handle the message if it wants
	if (fCurrentState) {
		AutoWriteLocker locker(fLocker);
		if (fLocker && !locker.IsLocked())
			return;

		Command* command = NULL;
		if (fCurrentState->MessageReceived(message, &command)) {
			Perform(command);
			return;
		}
	}

	switch (message->what) {
		case B_MODIFIERS_CHANGED:
			// NOTE: received only if the view has focus!!
			if (fCurrentState) {
				uint32 mods;
				if (message->FindInt32("modifiers", (int32*)&mods) != B_OK)
					mods = modifiers();
				fCurrentState->ModifiersChanged(mods);
				fMouseInfo.modifiers = mods;
			}
			break;
		default:
			BView::MessageReceived(message);
	}
}

// #pragma mark -

// MouseDown
void
StateView::MouseDown(BPoint where)
{
	if (fLocker && !fLocker->WriteLock())
		return;

	// query more info from the windows current message if available
	uint32 buttons;
	uint32 clicks;
	BMessage* message = Window() ? Window()->CurrentMessage() : NULL;
	if (!message || message->FindInt32("buttons", (int32*)&buttons) != B_OK)
		buttons = B_PRIMARY_MOUSE_BUTTON;
	if (!message || message->FindInt32("clicks", (int32*)&clicks) != B_OK)
		clicks = 1;

	if (fCurrentState)
		fCurrentState->MouseDown(where, buttons, clicks);

	// update mouse info *after* having called the ViewState hook
	fMouseInfo.buttons = buttons;
	fMouseInfo.position = where;

	if (fLocker)
		fLocker->WriteUnlock();
}

// MouseMoved
void
StateView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	if (fLocker && !fLocker->WriteLock())
		return;

	if (dragMessage && !fDropAnticipatingState) {
		// switch to a drop anticipating state if there is one available
		fDropAnticipatingState = StateForDragMessage(dragMessage);
		if (fDropAnticipatingState)
			fDropAnticipatingState->Init();
	}

	// TODO: I don't like this too much
	if (!dragMessage && fDropAnticipatingState) {
		fDropAnticipatingState->Cleanup();
		fDropAnticipatingState = NULL;
	}

	if (fDropAnticipatingState)
		fDropAnticipatingState->MouseMoved(where, transit, dragMessage);
	else {
		if (fCurrentState) {
			fCurrentState->MouseMoved(where, transit, dragMessage);
			if (fMouseInfo.buttons != 0)
				_TriggerUpdate();
		}
	}

	// update mouse info *after* having called the ViewState hook
	fMouseInfo.position = where;
	fMouseInfo.transit = transit;

	if (fLocker)
		fLocker->WriteUnlock();
}

// MouseUp
void
StateView::MouseUp(BPoint where)
{
	if (fLocker && !fLocker->WriteLock())
		return;

	if (fDropAnticipatingState) {
		Perform(fDropAnticipatingState->MouseUp());
		fDropAnticipatingState->Cleanup();
		fDropAnticipatingState = NULL;

		if (fCurrentState) {
			fCurrentState->MouseMoved(fMouseInfo.position, fMouseInfo.transit,
									  NULL);
		}
	} else {
		if (fCurrentState) {
			Perform(fCurrentState->MouseUp());
			_TriggerUpdate();
		}
	}

	// update mouse info *after* having called the ViewState hook
	fMouseInfo.buttons = 0;

	if (fLocker)
		fLocker->WriteUnlock();
}

// #pragma mark -

// KeyDown
void
StateView::KeyDown(const char* bytes, int32 numBytes)
{
	uint32 key;
	uint32 modifiers;
	BMessage* message = Window() ? Window()->CurrentMessage() : NULL;
	if (message
		&& message->FindInt32("raw_char", (int32*)&key) >= B_OK
		&& message->FindInt32("modifiers", (int32*)&modifiers) >= B_OK) {
		if (HandleKeyDown(key, modifiers))
			return;
	}
	BView::KeyDown(bytes, numBytes);
}

// KeyUp
void
StateView::KeyUp(const char* bytes, int32 numBytes)
{
	uint32 key;
	uint32 modifiers;
	BMessage* message = Window() ? Window()->CurrentMessage() : NULL;
	if (message
		&& message->FindInt32("raw_char", (int32*)&key) >= B_OK
		&& message->FindInt32("modifiers", (int32*)&modifiers) >= B_OK) {
		if (HandleKeyUp(key, modifiers))
			return;
	}
	BView::KeyUp(bytes, numBytes);
}


// #pragma mark -


status_t
StateView::Perform(perform_code code, void* data)
{
	return BView::Perform(code, data);
}


// #pragma mark -

// SetState
void
StateView::SetState(ViewState* state)
{
	if (fCurrentState == state)
		return;

	// switch states as appropriate
	if (fCurrentState)
		fCurrentState->Cleanup();

	fCurrentState = state;

	if (fCurrentState)
		fCurrentState->Init();
}

// UpdateStateCursor
void
StateView::UpdateStateCursor()
{
	if (!fCurrentState || !fCurrentState->UpdateCursor()) {
		SetViewCursor(B_CURSOR_SYSTEM_DEFAULT, true);
	}
}

// Draw
void
StateView::Draw(BView* into, BRect updateRect)
{
	if (fLocker && !fLocker->ReadLock()) {
		return;
	}

	if (fCurrentState)
		fCurrentState->Draw(into, updateRect);

	if (fDropAnticipatingState)
		fDropAnticipatingState->Draw(into, updateRect);

	if (fLocker)
		fLocker->ReadUnlock();
}

// MouseWheelChanged
bool
StateView::MouseWheelChanged(BPoint where, float x, float y)
{
	return false;
}

// HandleKeyDown
bool
StateView::HandleKeyDown(uint32 key, uint32 modifiers)
{
	// down't allow key events if mouse already pressed
	// (central place to prevent command stack mix up)
	if (fMouseInfo.buttons != 0)
		return false;

	AutoWriteLocker locker(fLocker);
	if (fLocker && !locker.IsLocked())
		return false;

	if (_HandleKeyDown(key, modifiers))
		return true;

	if (fCurrentState) {
		Command* command = NULL;
		if (fCurrentState->HandleKeyDown(key, modifiers, &command)) {
			Perform(command);
			return true;
		}
	}
	return false;
}

// HandleKeyUp
bool
StateView::HandleKeyUp(uint32 key, uint32 modifiers)
{
	// down't allow key events if mouse already pressed
	// (central place to prevent command stack mix up)
	if (fMouseInfo.buttons != 0)
		return false;

	AutoWriteLocker locker(fLocker);
	if (fLocker && !locker.IsLocked())
		return false;

	if (_HandleKeyUp(key, modifiers))
		return true;

	if (fCurrentState) {
		Command* command = NULL;
		if (fCurrentState->HandleKeyUp(key, modifiers, &command)) {
			Perform(command);
			return true;
		}
	}
	return false;
}

// FilterMouse
void
StateView::FilterMouse(BPoint* where) const
{
}

// StateForDragMessage
ViewState*
StateView::StateForDragMessage(const BMessage* message)
{
	return NULL;
}

// SetCommandStack
void
StateView::SetCommandStack(::CommandStack* stack)
{
	fCommandStack = stack;
}

// SetLocker
void
StateView::SetLocker(RWLocker* locker)
{
	fLocker = locker;
}

// SetUpdateTarget
void
StateView::SetUpdateTarget(BHandler* target, uint32 command)
{
	fUpdateTarget = target;
	fUpdateCommand = command;
}

// SetCatchAllEvents
void
StateView::SetCatchAllEvents(bool catchAll)
{
	if (fCatchAllEvents == catchAll)
		return;

	fCatchAllEvents = catchAll;

	if (fCatchAllEvents)
		_InstallEventFilter();
	else
		_RemoveEventFilter();
}

// Perform
status_t
StateView::Perform(Command* command)
{
	if (fCommandStack)
		return fCommandStack->Perform(command);

	// if there is no command stack, then nobody
	// else feels responsible...
	delete command;

	return B_NO_INIT;
}

// #pragma mark -

// _HandleKeyDown
bool
StateView::_HandleKeyDown(uint32 key, uint32 modifiers)
{
	return false;
}

// _HandleKeyUp
bool
StateView::_HandleKeyUp(uint32 key, uint32 modifiers)
{
	return false;
}

// _InstallEventFilter
void
StateView::_InstallEventFilter()
{
	if (!fCatchAllEvents)
		return;

	if (!fEventFilter)
		fEventFilter = new (nothrow) EventFilter(this);

	if (!fEventFilter || !Window())
		return;

	Window()->AddCommonFilter(fEventFilter);
}

void
StateView::_RemoveEventFilter()
{
	if (!fEventFilter || !Window())
		return;

	Window()->RemoveCommonFilter(fEventFilter);
}

// _TriggerUpdate
void
StateView::_TriggerUpdate()
{
	if (fUpdateTarget && fUpdateTarget->Looper()) {
		fUpdateTarget->Looper()->PostMessage(fUpdateCommand);
	}
}
