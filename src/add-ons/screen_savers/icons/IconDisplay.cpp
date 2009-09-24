/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/


#include "IconDisplay.h"

#include <stdio.h>
#include <stdlib.h>

#include <Bitmap.h>
#include <IconUtils.h>


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
IconDisplay::Run(VectorIcon* icon, BRect frame)
{
	delete fBitmap;

	fBitmap = new BBitmap(BRect(0, 0, frame.Width(), frame.Height()), 0,
		B_RGBA32);
	BIconUtils::GetVectorIcon(icon->data, icon->size, fBitmap);

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
			// Progressive showing
			if (fTicks < fDelay)
				backColor.alpha = (fTicks * 255) / fDelay;
			else
				fState++;
			break;

		case 1: 
			// Completed showing
			backColor.alpha = 255;
			fTicks = 0;
			fDelay = RAND_BETWEEN(STAY_TICKS_MIN, STAY_TICKS_MAX);
			fState++;
			break;

		case 2: 
			// Waiting
			if (fTicks < fDelay)
				return;
			fTicks = 0;
			backColor.alpha = 255;
			fDelay = RAND_BETWEEN(HIDE_TICKS_MIN, HIDE_TICKS_MAX);
			fState++;
			return;
			break;

		case 3: 
			// Progressive hiding
			if (fTicks < fDelay) {
				backColor.alpha = 255 - (fTicks * 255) / fDelay;
			} else {
				backColor.alpha = 0;
				fState++;
			}
			break;

		default: 
			// Finished
			fIsRunning = false;
			return;
			break;
	};

	view->SetHighColor(backColor);
	view->DrawBitmap(fBitmap, BPoint(fFrame.left, fFrame.top));
	backColor.alpha = 255;
	view->SetHighColor(backColor);
}
