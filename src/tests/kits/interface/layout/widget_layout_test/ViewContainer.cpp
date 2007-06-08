/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ViewContainer.h"

#include <Message.h>
#include <Window.h>


// internal messages
enum {
	MSG_LAYOUT_CONTAINER			= 'layc',
};


ViewContainer::ViewContainer(BRect frame)
	: BView(frame, "view container", B_FOLLOW_NONE, B_WILL_DRAW),
	  View(frame.OffsetToCopy(B_ORIGIN)),
	  fMouseFocus(NULL)
{
	BView::SetViewColor(B_TRANSPARENT_32_BIT);
	_AddedToContainer(this);
}


void
ViewContainer::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_LAYOUT_CONTAINER:
			View::Layout();
			break;
		default:
			BView::MessageReceived(message);
			break;
	}
}


void
ViewContainer::AllAttached()
{
	Window()->PostMessage(MSG_LAYOUT_CONTAINER, this);
}


void
ViewContainer::Draw(BRect updateRect)
{
	View::_Draw(this, updateRect);
}


void
ViewContainer::MouseDown(BPoint where)
{
	// get mouse buttons and modifiers
	uint32 buttons;
	int32 modifiers;
	_GetButtonsAndModifiers(buttons, modifiers);

	// get mouse focus
	if (!fMouseFocus && (buttons & B_PRIMARY_MOUSE_BUTTON)) {
		fMouseFocus = AncestorAt(where);
		if (fMouseFocus)
			SetMouseEventMask(B_POINTER_EVENTS);
	}

	// call hook
	if (fMouseFocus) {
		fMouseFocus->MouseDown(fMouseFocus->ConvertFromContainer(where),
			buttons, modifiers);
	}
}


void
ViewContainer::MouseUp(BPoint where)
{
	if (!fMouseFocus)
		return;

	// get mouse buttons and modifiers
	uint32 buttons;
	int32 modifiers;
	_GetButtonsAndModifiers(buttons, modifiers);

	// call hook
	if (fMouseFocus) {
		fMouseFocus->MouseUp(fMouseFocus->ConvertFromContainer(where),
			buttons, modifiers);
	}

	// unset the mouse focus when the primary button has been released
	if (!(buttons & B_PRIMARY_MOUSE_BUTTON))
		fMouseFocus = NULL;
}


void
ViewContainer::MouseMoved(BPoint where, uint32 code, const BMessage* message)
{
	if (!fMouseFocus)
		return;

	// get mouse buttons and modifiers
	uint32 buttons;
	int32 modifiers;
	_GetButtonsAndModifiers(buttons, modifiers);

	// call hook
	if (fMouseFocus) {
		fMouseFocus->MouseMoved(fMouseFocus->ConvertFromContainer(where),
			buttons, modifiers);
	}
}


void
ViewContainer::InvalidateLayout(bool descendants)
{
	BView::InvalidateLayout(descendants);
}


void
ViewContainer::InvalidateLayout()
{
	if (View::IsLayoutValid()) {
		View::InvalidateLayout();

		// trigger asynchronous re-layout
		if (Window())
			Window()->PostMessage(MSG_LAYOUT_CONTAINER, this);
	}
}


void
ViewContainer::Draw(BView* container, BRect updateRect)
{
}


void
ViewContainer::MouseDown(BPoint where, uint32 buttons, int32 modifiers)
{
}


void
ViewContainer::MouseUp(BPoint where, uint32 buttons, int32 modifiers)
{
}


void
ViewContainer::MouseMoved(BPoint where, uint32 buttons, int32 modifiers)
{
}


void
ViewContainer::_GetButtonsAndModifiers(uint32& buttons, int32& modifiers)
{
	buttons = 0;
	modifiers = 0;

	if (BMessage* message = Window()->CurrentMessage()) {
		if (message->FindInt32("buttons", (int32*)&buttons) != B_OK)
			buttons = 0;
		if (message->FindInt32("modifiers", modifiers) != B_OK)
			modifiers = 0;
	}
}
