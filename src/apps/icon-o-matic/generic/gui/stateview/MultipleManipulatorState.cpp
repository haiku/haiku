/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "MultipleManipulatorState.h"

#include <stdio.h>

#include <AppDefs.h>

#include "Manipulator.h"
#include "StateView.h"

// constructor
MultipleManipulatorState::MultipleManipulatorState(StateView* view)
	: ViewState(view),
	  fManipulators(24),
	  fCurrentManipulator(NULL),
	  fPreviousManipulator(NULL)
{
}

// destructor
MultipleManipulatorState::~MultipleManipulatorState()
{
	DeleteManipulators();
}

// #pragma mark -

// Init
void
MultipleManipulatorState::Init()
{
}

// Cleanup
void
MultipleManipulatorState::Cleanup()
{
}

// #pragma mark -

// Draw
void
MultipleManipulatorState::Draw(BView* into, BRect updateRect)
{
	int32 count = fManipulators.CountItems();
	for (int32 i = 0; i < count; i++) {
		Manipulator* manipulator =
			(Manipulator*)fManipulators.ItemAtFast(i);
		if (manipulator->Bounds().Intersects(updateRect))
			manipulator->Draw(into, updateRect);
	}
}

// MessageReceived
bool
MultipleManipulatorState::MessageReceived(BMessage* message,
										  Command** _command)
{
	int32 count = fManipulators.CountItems();
	for (int32 i = 0; i < count; i++) {
		Manipulator* manipulator =
			(Manipulator*)fManipulators.ItemAtFast(i);
		if (manipulator->MessageReceived(message, _command))
			return true;
	}
	return false;
}

// #pragma mark -

// MouseDown
void
MultipleManipulatorState::MouseDown(BPoint where, uint32 buttons, uint32 clicks)
{
	if (buttons & B_SECONDARY_MOUSE_BUTTON) {
		_ShowContextMenu(where);
		return;
	}

	if (clicks == 2
		&& fPreviousManipulator
		&& fManipulators.HasItem(fPreviousManipulator)) {
		// valid double click (onto the same, still existing manipulator)
		if (fPreviousManipulator->TrackingBounds(fView).Contains(where)
			&& fPreviousManipulator->DoubleClicked(where)) {
			// TODO: eat the click here or wait for MouseUp?
			fPreviousManipulator = NULL;
			return;
		}
	}

	int32 count = fManipulators.CountItems();
	for (int32 i = count - 1; i >= 0; i--) {
		Manipulator* manipulator =
			(Manipulator*)fManipulators.ItemAtFast(i);
		if (manipulator->MouseDown(where)) {
			fCurrentManipulator = manipulator;
			break;
		}
	}

	fView->SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
}

// MouseMoved
void
MultipleManipulatorState::MouseMoved(BPoint where, uint32 transit,
									 const BMessage* dragMessage)
{
	if (fCurrentManipulator) {
		// the mouse is currently pressed
		fCurrentManipulator->MouseMoved(where);

	} else {
		// the mouse is currently NOT pressed

		// call MouseOver on all manipulators
		// until one feels responsible
		int32 count = fManipulators.CountItems();
		bool updateCursor = true;
		for (int32 i = 0; i < count; i++) {
			Manipulator* manipulator =
				(Manipulator*)fManipulators.ItemAtFast(i);
			if (manipulator->TrackingBounds(fView).Contains(where)
				&& manipulator->MouseOver(where)) {
				updateCursor = false;
				break;
			}
		}
		if (updateCursor)
			_UpdateCursor();
	}
}

// MouseUp
Command*
MultipleManipulatorState::MouseUp()
{
	Command* command = NULL;
	if (fCurrentManipulator) {
		command = fCurrentManipulator->MouseUp();
		fPreviousManipulator = fCurrentManipulator;
		fCurrentManipulator = NULL;
	}
	return command;
}

// #pragma mark -

// ModifiersChanged
void
MultipleManipulatorState::ModifiersChanged(uint32 modifiers)
{
	int32 count = fManipulators.CountItems();
	for (int32 i = 0; i < count; i++) {
		Manipulator* manipulator =
			(Manipulator*)fManipulators.ItemAtFast(i);
		manipulator->ModifiersChanged(modifiers);
	}
}

// HandleKeyDown
bool
MultipleManipulatorState::HandleKeyDown(uint32 key, uint32 modifiers,
										Command** _command)
{
	// TODO: somehow this looks suspicious, because it doesn't
	// seem guaranteed that the manipulator having indicated to
	// handle the key down handles the matching key up event...
	// maybe there should be the concept of the "focused manipulator"
	int32 count = fManipulators.CountItems();
	for (int32 i = 0; i < count; i++) {
		Manipulator* manipulator =
			(Manipulator*)fManipulators.ItemAtFast(i);
		if (manipulator->HandleKeyDown(key, modifiers, _command))
			return true;
	}
	return false;
}

// HandleKeyUp
bool
MultipleManipulatorState::HandleKeyUp(uint32 key, uint32 modifiers,
									  Command** _command)
{
	int32 count = fManipulators.CountItems();
	for (int32 i = 0; i < count; i++) {
		Manipulator* manipulator =
			(Manipulator*)fManipulators.ItemAtFast(i);
		if (manipulator->HandleKeyUp(key, modifiers, _command))
			return true;
	}
	return false;
}

// UpdateCursor
bool
MultipleManipulatorState::UpdateCursor()
{
	if (fPreviousManipulator && fManipulators.HasItem(fPreviousManipulator))
		return fPreviousManipulator->UpdateCursor();
	return false;
}

// #pragma mark -

// AddManipulator
bool
MultipleManipulatorState::AddManipulator(Manipulator* manipulator)
{
	if (!manipulator)
		return false;

	if (fManipulators.AddItem((void*)manipulator)) {
		manipulator->AttachedToView(fView);
		fView->Invalidate(manipulator->Bounds());
		return true;
	}
	return false;
}

// RemoveManipulator
Manipulator*
MultipleManipulatorState::RemoveManipulator(int32 index)
{
	Manipulator* manipulator = (Manipulator*)fManipulators.RemoveItem(index);

	if (manipulator == fCurrentManipulator)
		fCurrentManipulator = NULL;

	if (manipulator) {
		fView->Invalidate(manipulator->Bounds());
		manipulator->DetachedFromView(fView);
	}

	return manipulator;
}

// DeleteManipulators
void
MultipleManipulatorState::DeleteManipulators()
{
	BRect dirty(LONG_MAX, LONG_MAX, LONG_MIN, LONG_MIN);

	int32 count = fManipulators.CountItems();
	for (int32 i = 0; i < count; i++) {
		Manipulator* m = (Manipulator*)fManipulators.ItemAtFast(i);
		dirty = dirty | m->Bounds();
		m->DetachedFromView(fView);
		delete m;
	}
	fManipulators.MakeEmpty();
	fCurrentManipulator = NULL;
	fPreviousManipulator = NULL;

	fView->Invalidate(dirty);
	_UpdateCursor();
}

// CountManipulators
int32
MultipleManipulatorState::CountManipulators() const
{
	return fManipulators.CountItems();
}

// ManipulatorAt
Manipulator*
MultipleManipulatorState::ManipulatorAt(int32 index) const
{
	return (Manipulator*)fManipulators.ItemAt(index);
}

// ManipulatorAtFast
Manipulator*
MultipleManipulatorState::ManipulatorAtFast(int32 index) const
{
	return (Manipulator*)fManipulators.ItemAtFast(index);
}

// #pragma mark -

// _UpdateViewCursor
void
MultipleManipulatorState::_UpdateCursor()
{
	if (fCurrentManipulator)
		fCurrentManipulator->UpdateCursor();
	else
		fView->SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
}

// _ShowContextMenu
void
MultipleManipulatorState::_ShowContextMenu(BPoint where)
{
	int32 count = fManipulators.CountItems();
	for (int32 i = 0; i < count; i++) {
		Manipulator* manipulator =
			(Manipulator*)fManipulators.ItemAtFast(i);
		if (manipulator->ShowContextMenu(where))
			return;
	}
}


