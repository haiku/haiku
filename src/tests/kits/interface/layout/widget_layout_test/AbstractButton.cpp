/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "AbstractButton.h"

#include <View.h>


// #pragma mark - AbstractButton


AbstractButton::AbstractButton(button_policy policy, BMessage* message,
	BMessenger target)
	: View(BRect(0, 0, 0, 0)),
	  BInvoker(message, target),
	  fPolicy(policy),
	  fSelected(false),
	  fPressed(false),
	  fPressedInBounds(false)
{
}


void
AbstractButton::SetPolicy(button_policy policy)
{
	fPolicy = policy;
}


button_policy
AbstractButton::Policy() const
{
	return fPolicy;
}


void
AbstractButton::SetSelected(bool selected)
{
	if (selected != fSelected) {
		fSelected = selected;
		Invalidate();

		// check whether to notify the listeners depending on the button policy
		bool notify = false;
		switch (fPolicy) {
			case BUTTON_POLICY_TOGGLE_ON_RELEASE:
			case BUTTON_POLICY_SELECT_ON_RELEASE:
				// always notify on selection changes
				notify = true;
				break;
			case BUTTON_POLICY_INVOKE_ON_RELEASE:
				// only notify when the user interaction has been finished
				notify = !fPressed;
				break;
		}

		if (notify) {
			// notify synchronous listeners
			_NotifyListeners();

			// send the message
			if (Message()) {
				BMessage message(*Message());
				message.AddBool("selected", IsSelected());
				InvokeNotify(&message);
			}
		}
	}
}


bool
AbstractButton::IsSelected() const
{
	return fSelected;
}


void
AbstractButton::MouseDown(BPoint where, uint32 buttons, int32 modifiers)
{
	if (fPressed)
		return;

	fPressed = true;
	_PressedUpdate(where);
}


void
AbstractButton::MouseUp(BPoint where, uint32 buttons, int32 modifiers)
{
	if (!fPressed || (buttons & B_PRIMARY_MOUSE_BUTTON))
		return;

	_PressedUpdate(where);
	fPressed = false;
	if (fPressedInBounds) {
		fPressedInBounds = false;

		// update selected state according to policy
		bool selected = fSelected;
		switch (fPolicy) {
			case BUTTON_POLICY_TOGGLE_ON_RELEASE:
				selected = !fSelected;
				break;
			case BUTTON_POLICY_SELECT_ON_RELEASE:
				selected = true;
				break;
			case BUTTON_POLICY_INVOKE_ON_RELEASE:
				selected = false;
				break;
		}

		SetSelected(selected);
	}
	Invalidate();
}


void
AbstractButton::MouseMoved(BPoint where, uint32 buttons, int32 modifiers)
{
	if (!fPressed)
		return;

	_PressedUpdate(where);
}


void
AbstractButton::AddListener(Listener* listener)
{
	if (listener && !fListeners.HasItem(listener))
		fListeners.AddItem(listener);
}


void
AbstractButton::RemoveListener(Listener* listener)
{
	if (listener)
		fListeners.RemoveItem(listener);
}


bool
AbstractButton::IsPressed() const
{
	return (fPressed && fPressedInBounds);
}


void
AbstractButton::_PressedUpdate(BPoint where)
{
	bool pressedInBounds = Bounds().Contains(where);
	if (pressedInBounds == fPressedInBounds)
		return;

	fPressedInBounds = pressedInBounds;

	// update the selected state according to the button policy
	switch (fPolicy) {
		case BUTTON_POLICY_TOGGLE_ON_RELEASE:
		case BUTTON_POLICY_SELECT_ON_RELEASE:
			// nothing to do
			break;
		case BUTTON_POLICY_INVOKE_ON_RELEASE:
			SetSelected(fPressedInBounds);
			break;
	}

	Invalidate();
}


void
AbstractButton::_NotifyListeners()
{
	if (!fListeners.IsEmpty()) {
		BList listeners(fListeners);
		for (int32 i = 0; Listener* listener = (Listener*)listeners.ItemAt(i);
			 i++) {
			listener->SelectionChanged(this);
		}
	}
}


// #pragma mark - AbstractButton::Listener


AbstractButton::Listener::~Listener()
{
}

