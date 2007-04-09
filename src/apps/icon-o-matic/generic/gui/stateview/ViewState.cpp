/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "ViewState.h"

#include "StateView.h"

mouse_info::mouse_info()
	: buttons(0),
	  position(-1, -1),
	  transit(B_OUTSIDE_VIEW),
	  modifiers(::modifiers())
{
}

// constructor
ViewState::ViewState(StateView* view)
	: fView(view),
	  fMouseInfo(view->MouseInfo())
{
}

// constructor
ViewState::ViewState(const ViewState& other)
	: fView(other.fView),
	  fMouseInfo(other.fMouseInfo)
{
}

// destructor
ViewState::~ViewState()
{
}

// #pragma mark -

// Init
void
ViewState::Init()
{
}

// Cleanup
void
ViewState::Cleanup()
{
}

// #pragma mark -

// Draw
void
ViewState::Draw(BView* into, BRect updateRect)
{
}

// MessageReceived
bool
ViewState::MessageReceived(BMessage* message, Command** _command)
{
	return false;
}

// #pragma mark -

// MouseDown
void
ViewState::MouseDown(BPoint where, uint32 buttons, uint32 clicks)
{
}

// MouseMoved
void
ViewState::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
}

// MouseUp
Command*
ViewState::MouseUp()
{
	return NULL;
}

// #pragma mark -

// ModifiersChanged
void
ViewState::ModifiersChanged(uint32 modifiers)
{
}

// HandleKeyDown
bool
ViewState::HandleKeyDown(uint32 key, uint32 modifiers, Command** _command)
{
	return false;
}

// HandleKeyUp
bool
ViewState::HandleKeyUp(uint32 key, uint32 modifiers, Command** _command)
{
	return false;
}

// UpdateCursor
bool
ViewState::UpdateCursor()
{
	return false;
}
