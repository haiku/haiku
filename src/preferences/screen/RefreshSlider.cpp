/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Rafael Romo
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "RefreshSlider.h"
#include "Constants.h"

#include <Catalog.h>
#include <String.h>
#include <Window.h>

#include <new>
#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Screen"


RefreshSlider::RefreshSlider(BRect frame, float min, float max, uint32 resizingMode)
	: BSlider(frame, B_TRANSLATE("Screen"), B_TRANSLATE("Refresh rate:"),
		new BMessage(SLIDER_INVOKE_MSG), (int32)rintf(min * 10), (int32)rintf(max * 10),
		B_BLOCK_THUMB, resizingMode),
	fStatus(new (std::nothrow) char[32])
{
	BString minRefresh;
	minRefresh << (uint32)min;
	BString maxRefresh;
	maxRefresh << (uint32)max;
	SetLimitLabels(minRefresh.String(), maxRefresh.String());

	SetHashMarks(B_HASH_MARKS_BOTTOM);
	SetHashMarkCount(uint32(max - min) / 5 + 1);

	SetKeyIncrementValue(1);
}


RefreshSlider::~RefreshSlider()
{
	delete[] fStatus;
}


void
RefreshSlider::DrawFocusMark()
{
	if (IsFocus()) {
		rgb_color blue = { 0, 0, 229, 255 };
		
		BRect rect(ThumbFrame());		
		BView *view = OffscreenView();
				
		rect.InsetBy(2.0, 2.0);
		rect.right--;
		rect.bottom--;
		
		view->SetHighColor(blue);
		view->StrokeRect(rect);
	}
}


void
RefreshSlider::KeyDown(const char *bytes, int32 numBytes)
{
	switch (*bytes) {
		case B_LEFT_ARROW:
		{
			SetValue(Value() - 1);
			Invoke();
			break;
		}
		
		case B_RIGHT_ARROW:
		{
			SetValue(Value() + 1);
			Invoke();
			break;
		}

		default:
			break;
	}
}


const char*
RefreshSlider::UpdateText() const
{
	if (fStatus != NULL)
		snprintf(fStatus, 32, B_TRANSLATE("%.1f Hz"), (float)Value() / 10);

	return fStatus;
}
