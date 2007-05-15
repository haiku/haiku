/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "CheckBox.h"

#include <View.h>


CheckBox::CheckBox(BMessage* message, BMessenger target)
	: View(BRect(0, 0, 12, 12)),
	  BInvoker(message, target),
	  fSelected(false),
	  fPressed(false)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void
CheckBox::SetSelected(bool selected)
{
	if (selected != fSelected) {
		fSelected = selected;
		Invalidate();

		// send the message
		if (Message()) {
			BMessage message(*Message());
			message.AddBool("selected", IsSelected());
			InvokeNotify(&message);
		}
	}
}


bool
CheckBox::IsSelected() const
{
	return fSelected;
}


BSize
CheckBox::MinSize()
{
	return BSize(12, 12);
}


BSize
CheckBox::MaxSize()
{
	return MinSize();
}


void
CheckBox::Draw(BView* container, BRect updateRect)
{
	BRect rect(Bounds());

	if (fPressed)
		container->SetHighColor((rgb_color){ 120, 0, 0, 255 });
	else
		container->SetHighColor((rgb_color){ 0, 0, 0, 255 });

	container->StrokeRect(rect);

	if (fPressed ? fPressedSelected : fSelected) {
		rect.InsetBy(2, 2);
		container->StrokeLine(rect.LeftTop(), rect.RightBottom());
		container->StrokeLine(rect.RightTop(), rect.LeftBottom());
	}
}


void
CheckBox::MouseDown(BPoint where, uint32 buttons, int32 modifiers)
{
	if (fPressed)
		return;

	fPressed = true;
	fPressedSelected = fSelected;
	_PressedUpdate(where);
}


void
CheckBox::MouseUp(BPoint where, uint32 buttons, int32 modifiers)
{
	if (!fPressed || (buttons & B_PRIMARY_MOUSE_BUTTON))
		return;

	_PressedUpdate(where);
	fPressed = false;
	SetSelected(fPressedSelected);
	Invalidate();
}


void
CheckBox::MouseMoved(BPoint where, uint32 buttons, int32 modifiers)
{
	if (!fPressed)
		return;

	_PressedUpdate(where);
}


void
CheckBox::_PressedUpdate(BPoint where)
{
	bool pressedSelected = Bounds().Contains(where) ^ fSelected;
	if (pressedSelected != fPressedSelected) {
		fPressedSelected = pressedSelected;
		Invalidate();
	}
}
