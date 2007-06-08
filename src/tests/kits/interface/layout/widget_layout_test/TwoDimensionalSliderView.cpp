/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TwoDimensionalSliderView.h"

#include <View.h>


TwoDimensionalSliderView::TwoDimensionalSliderView(BMessage* message,
	BMessenger target)
	: View(BRect(0, 0, 4, 4)),
	  BInvoker(message, target),
	  fMinLocation(0, 0),
	  fMaxLocation(0, 0),
	  fDragging(false)
{
	SetViewColor((rgb_color){255, 0, 0, 255});
}


void
TwoDimensionalSliderView::SetLocationRange(BPoint minLocation,
	BPoint maxLocation)
{
	if (maxLocation.x < minLocation.x)
		maxLocation.x = minLocation.x;
	if (maxLocation.y < minLocation.y)
		maxLocation.y = minLocation.y;

	fMinLocation = minLocation;
	fMaxLocation = maxLocation;

	// force valid value
	SetValue(Value());
}


BPoint
TwoDimensionalSliderView::MinLocation() const
{
	return fMinLocation;
}


BPoint
TwoDimensionalSliderView::MaxLocation() const
{
	return fMaxLocation;
}


BPoint
TwoDimensionalSliderView::Value() const
{
	return Location() - fMinLocation;
}


void
TwoDimensionalSliderView::SetValue(BPoint value)
{
	BPoint location = fMinLocation + value;
	if (location.x < fMinLocation.x)
		location.x = fMinLocation.x;
	if (location.y < fMinLocation.y)
		location.y = fMinLocation.y;
	if (location.x > fMaxLocation.x)
		location.x = fMaxLocation.x;
	if (location.y > fMaxLocation.y)
		location.y = fMaxLocation.y;

	if (location != Location()) {
		SetFrame(Frame().OffsetToCopy(location));

		// send the message
		if (Message()) {
			BMessage message(*Message());
			message.AddPoint("value", Value());
			InvokeNotify(&message);
		}
	}
}


void
TwoDimensionalSliderView::MouseDown(BPoint where, uint32 buttons,
	int32 modifiers)
{
	if (fDragging)
		return;

	fOriginalLocation = Frame().LeftTop();
	fOriginalPoint = ConvertToContainer(where);
	fDragging = true;
}


void
TwoDimensionalSliderView::MouseUp(BPoint where, uint32 buttons, int32 modifiers)
{
	if (!fDragging || (buttons & B_PRIMARY_MOUSE_BUTTON))
		return;

	fDragging = false;
}


void
TwoDimensionalSliderView::MouseMoved(BPoint where, uint32 buttons,
	int32 modifiers)
{
	if (!fDragging)
		return;

	BPoint moved = ConvertToContainer(where) - fOriginalPoint;
	SetValue(fOriginalLocation - fMinLocation + moved);
}
