/*
 * Copyright 1999-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */
 
 
#include "ResizableButton.h"

ResizableButton::ResizableButton(BRect parentFrame, BRect frame, 
	const char* name, const char* label, BMessage* msg)
	:
	BButton(frame, name, label, msg, B_FOLLOW_BOTTOM)
{
	float width = parentFrame.right - parentFrame.left;
	float height = parentFrame.bottom - parentFrame.top;
	fPercentages.left = frame.left / width;
	fPercentages.top = frame.top / height;
	fPercentages.right = frame.right / width;
	fPercentages.bottom = frame.bottom / height;
}
 

void
ResizableButton::ChangeToNewSize(float newWidth, float newHeight)
{
	float newX = fPercentages.left* newWidth;
	float newW = (fPercentages.right* newWidth) - newX;
	BRect b = Frame();
	MoveBy(newX - b.left, 0);
	ResizeTo(newW, b.bottom - b.top); 
	Invalidate();
}
