/*
 * Copyright (c) 2006-2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */
#ifndef _DRAW_BUTTON_H
#define _DRAW_BUTTON_H

#include <Looper.h>
#include <Application.h>
#include <Window.h>
#include <Button.h>
#include <Bitmap.h>
#include <Rect.h>


class DrawButton : public BButton
{
public:
					DrawButton(BRect frame, const char *name, BBitmap *up, 
						BBitmap *down,BMessage *msg, int32 resize,
						int32 flags);
	virtual			~DrawButton(void);
	
			void	Draw(BRect update);

			void 	SetBitmaps(BBitmap *up, BBitmap *down);
			void 	ResizeToPreferred(void);
			void 	SetDisabled(BBitmap *disabled);
	
private:
	
	BBitmap	*fUp,
			*fDown,
			*fDisabled;
};

#endif
