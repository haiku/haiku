/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ImageButton.h"

#include <Bitmap.h>


ImageButton::ImageButton(BRect frame, const char* name, BBitmap* image,
	BMessage* message, uint32 resizingMode = B_FOLLOW_NONE)
	:
	BButton(frame, name, "", message, resizingMode)
{
	fImage = image;

	fDrawPoint.x = ((frame.RightTop().x - frame.LeftTop().x)
		- (image->Bounds().RightBottom().x + 1)) / 2;

	fDrawPoint.y = ((frame.LeftBottom().y - frame.LeftTop().y)
		- (image->Bounds().RightBottom().y + 1)) / 2;

	fInnerBounds = Bounds();
	fInnerBounds.InsetBy(3, 3);

	SetDrawingMode(B_OP_ALPHA);
}


ImageButton::~ImageButton()
{

}


void
ImageButton::Draw(BRect updateRect)
{
	BButton::Draw(updateRect);
	DrawBitmap(fImage, fDrawPoint);

	if (!IsEnabled()) {
		rgb_color tempColor = HighColor();
		SetHighColor(255, 255, 255, 155);
		FillRect(fInnerBounds, B_SOLID_HIGH);
		SetHighColor(tempColor);
	}
}


void
ImageButton::ResizeTo(float width, float height)
{
	BButton::ResizeTo(width, height);
	fInnerBounds = Bounds();
	fInnerBounds.InsetBy(3, 3);
}


void
ImageButton::SetBitmap(BBitmap* image)
{
	fImage = image;
	Invalidate();
}
