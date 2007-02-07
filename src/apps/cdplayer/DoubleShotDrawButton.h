/*
 * Copyright (c) 2006-2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */
#ifndef _DOUBLESHOT_DRAW_BUTTON_H
#define _DOUBLESHOT_DRAW_BUTTON_H

#include <Looper.h>
#include <Application.h>
#include <Window.h>
#include <Button.h>
#include <Bitmap.h>
#include <Rect.h>
#include "DrawButton.h"

class DoubleShotDrawButton : public DrawButton
{
public:
			DoubleShotDrawButton(BRect frame, const char *name, BBitmap *up, 
								BBitmap *down,BMessage *msg, int32 resize,
								int32 flags);
	
	void	MouseDown(BPoint point);
};

#endif
