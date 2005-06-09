/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Inspired by SoundCapture from Be newsletter (Media Kit Basics: Consumers and Producers)
 */
#ifndef UPDOWNBUTTON_H
#define UPDOWNBUTTON_H

#include <Bitmap.h>
#include <Control.h>

#define DRAG_ITEM 'dndi'

class UpDownButton : public BControl
{
public:
	UpDownButton(BRect rect, BMessage *msg, uint32 resizeFlags = 0);
	~UpDownButton();
	virtual void Draw(BRect);
	virtual void MouseDown(BPoint point);
	virtual void MouseMoved(BPoint point, uint32 transit, const BMessage *message);
	virtual void MouseUp(BPoint point);

private:
	BBitmap *fBitmapUp, *fBitmapDown, *fBitmapMiddle;	
	float fTrackingY;
	int32 fLastValue;
};

#endif
