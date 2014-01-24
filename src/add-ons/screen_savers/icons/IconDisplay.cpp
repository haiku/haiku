/*
 * Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
 * Copyright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Vincent Duvert, vincent.duvert@free.fr
 *		John Scipione, jscipione@gmail.com
 */


#include "IconDisplay.h"

#include <stdlib.h>

#include <Bitmap.h>
#include <IconUtils.h>
#include <View.h>

#include "VectorIcon.h"


#define RAND_BETWEEN(a, b) ((rand() % ((b) - (a) + 1) + (a)))
#define SHOW_TICKS_MIN 20
#define SHOW_TICKS_MAX 50
#define STAY_TICKS_MIN 50
#define STAY_TICKS_MAX 100
#define HIDE_TICKS_MIN 20
#define HIDE_TICKS_MAX 50


IconDisplay::IconDisplay()
	:
	fIsRunning(false),
	fBitmap(NULL)
{
}


IconDisplay::~IconDisplay()
{
	delete fBitmap;
}


void
IconDisplay::Run(vector_icon* icon, BRect frame)
{
	delete fBitmap;

	fBitmap = new BBitmap(BRect(0, 0, frame.Width(), frame.Height()), 0,
		B_RGBA32);
	if (BIconUtils::GetVectorIcon(icon->data, icon->size, fBitmap) != B_OK)
		return;

	fState = 0;
	fTicks = 0;
	fDelay = RAND_BETWEEN(SHOW_TICKS_MIN, SHOW_TICKS_MAX);
	fFrame = frame;

	fIsRunning = true;
}


void
IconDisplay::ClearOn(BView* view)
{
	if (!fIsRunning || fState == 2)
		return;

	view->FillRect(fFrame);
}


void
IconDisplay::DrawOn(BView* view, uint32 delta)
{
	if (!fIsRunning)
		return;

	fTicks += delta;

	rgb_color backColor = view->HighColor();

	switch (fState) {
		case 0:
			// progressive showing
			if (fTicks < fDelay)
				backColor.alpha = (fTicks * 255) / fDelay;
			else
				fState++;

			break;

		case 1:
			// completed showing
			backColor.alpha = 255;
			fTicks = 0;
			fDelay = RAND_BETWEEN(STAY_TICKS_MIN, STAY_TICKS_MAX);
			fState++;
			break;

		case 2:
			// waiting
			if (fTicks < fDelay)
				return;

			fTicks = 0;
			backColor.alpha = 255;
			fDelay = RAND_BETWEEN(HIDE_TICKS_MIN, HIDE_TICKS_MAX);
			fState++;
			return;
			break;

		case 3:
			// progressive hiding
			if (fTicks < fDelay) {
				backColor.alpha = 255 - (fTicks * 255) / fDelay;
			} else {
				backColor.alpha = 0;
				fState++;
			}
			break;

		default:
			// finished
			fIsRunning = false;
			return;
			break;
	};

	view->SetHighColor(backColor);
	view->DrawBitmap(fBitmap, BPoint(fFrame.left, fFrame.top));
	backColor.alpha = 255;
	view->SetHighColor(backColor);
}
