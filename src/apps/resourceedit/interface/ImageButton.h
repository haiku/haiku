/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef IMAGE_BUTTON_H
#define IMAGE_BUTTON_H


#include <Button.h>


class ImageButton : public BButton
{
public:
				ImageButton(BRect frame, const char* name, BBitmap* image,
					BMessage* message, uint32 resizingMode);
				~ImageButton();

	void		Draw(BRect updateRect);
	void		ResizeTo(float width, float height);

	void		SetBitmap(BBitmap* image);

private:
	BBitmap*	fImage;
	BPoint		fDrawPoint;
	BRect		fInnerBounds;

};


#endif
