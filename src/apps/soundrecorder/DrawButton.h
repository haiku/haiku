/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Reworked from DarkWyrm version in CDPlayer
 */

#ifndef _DRAW_BUTTON_H
#define _DRAW_BUTTON_H

#include <Control.h>
#include <Bitmap.h>

class DrawButton : public BControl
{
public:
			DrawButton(BRect frame, const char *name, const unsigned char *on, 
						const unsigned char *off, BMessage *msg, int32 resize = 0,
						int32 flags = 0);
			~DrawButton(void);
	
	void	Draw(BRect update);
	void 	AttachedToWindow();
	void	MouseUp(BPoint pt);
	bool	ButtonState() {	return fButtonState; };
private:
	
	BBitmap	fOn;
	BBitmap	fOff;
	bool fButtonState;
};

#endif
