/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "Manipulator.h"

#include "Observable.h"

// constructor
Manipulator::Manipulator(Observable* object)
	: Observer(),
	  fManipulatedObject(object)
{
	if (fManipulatedObject)
		fManipulatedObject->AddObserver(this);
}

// destructor
Manipulator::~Manipulator()
{
	if (fManipulatedObject)
		fManipulatedObject->RemoveObserver(this);
}

// #pragma mark -

// Draw
void
Manipulator::Draw(BView* into, BRect updateRect)
{
}

// MouseDown
bool
Manipulator::MouseDown(BPoint where)
{
	return false;
}

// MouseMoved
void
Manipulator::MouseMoved(BPoint where)
{
}

// MouseUp
Command*
Manipulator::MouseUp()
{
	return NULL;
}

// MouseOver
bool
Manipulator::MouseOver(BPoint where)
{
	return false;
}

// DoubleClicked
bool
Manipulator::DoubleClicked(BPoint where)
{
	return false;
}

// ShowContextMenu
bool
Manipulator::ShowContextMenu(BPoint where)
{
	return false;
}

// #pragma mark -

bool
Manipulator::MessageReceived(BMessage* message, Command** _command)
{
	return false;
}

// #pragma mark -

// ModifiersChanged
void
Manipulator::ModifiersChanged(uint32 modifiers)
{
}

// HandleKeyDown
bool
Manipulator::HandleKeyDown(uint32 key, uint32 modifiers, Command** _command)
{
	return false;
}

// HandleKeyUp
bool
Manipulator::HandleKeyUp(uint32 key, uint32 modifiers, Command** _command)
{
	return false;
}

// UpdateCursor
bool
Manipulator::UpdateCursor()
{
	return false;
}

// #pragma mark -

// TrackingBounds
BRect
Manipulator::TrackingBounds(BView* withinView)
{
	return Bounds();
}

// AttachedToView
void
Manipulator::AttachedToView(BView* view)
{
}

// DetachedFromView
void
Manipulator::DetachedFromView(BView* view)
{
}
