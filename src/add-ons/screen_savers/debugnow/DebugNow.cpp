/*
 * Copyright 2007-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ryan Leavengood, leavengood@gmail.com
 */


#include "DebugNow.h"

#include <Catalog.h>
#include <Font.h>
#include <StringView.h>
#include <View.h>

#include <BuildScreenSaverDefaultSettingsView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Screensaver DebugNow"


static const rgb_color kMediumBlue		= { 0, 0, 100 };
static const rgb_color kWhite			= { 255, 255, 255 };


//	#pragma mark - Instantiation function


BScreenSaver* instantiate_screen_saver(BMessage* message, image_id image)
{
	return new DebugNow(message, image);
}


//	#pragma mark - DebugNow


DebugNow::DebugNow(BMessage* archive, image_id id)
	:
	BScreenSaver(archive, id),
	fLine1(B_TRANSLATE_COMMENT("DEBUG",
		"keep it short and all uppercase, 5 characters or less")),
	fLine2(B_TRANSLATE_COMMENT("NOW",
		"keep it short and all uppercase, 5 characters or less"))
{
}


void
DebugNow::StartConfig(BView* view)
{
	BPrivate::BuildScreenSaverDefaultSettingsView(view, "DEBUG NOW",
		B_TRANSLATE("by Ryan Leavengood"));
}


status_t
DebugNow::StartSaver(BView* view, bool preview)
{
	float viewWidth = view->Bounds().Width();
	float viewHeight = view->Bounds().Height();

	BFont font;
	view->GetFont(&font);
	font.SetSize(viewHeight / 3);
	view->SetFont(&font);

	fDelta.nonspace = 0;
	fDelta.space = 0;
	BRect stringRect;
	font.GetBoundingBoxesForStrings(&fLine1, 1, B_SCREEN_METRIC, &fDelta,
		&stringRect);
	float y = ((viewHeight - (stringRect.Height() * 2 + viewHeight / 10)) / 2)
		+ stringRect.Height();
	fLine1Start.Set(((viewWidth - stringRect.Width()) / 2) - stringRect.left, y);
	font.GetBoundingBoxesForStrings(&fLine2, 1, B_SCREEN_METRIC, &fDelta,
		&stringRect);
	fLine2Start.Set(((viewWidth - stringRect.Width()) / 2) - stringRect.left,
		y + stringRect.Height() + viewHeight / 10);

	// set tick size to 500,000 microseconds (0.5 seconds)
	SetTickSize(500000);

	return B_OK;
}


void
DebugNow::Draw(BView* view, int32 frame)
{
	// on first frame set the low color to make the text rendering correct
	if (frame == 0)
		view->SetLowColor(kMediumBlue);

	// draw the background color every frame
	view->SetHighColor(kMediumBlue);
	view->FillRect(view->Bounds());

	// draw the text every other frame to make the it blink
	if (frame % 2 == 1) {
		view->SetHighColor(kWhite);
		view->DrawString(fLine1, fLine1Start, &fDelta);
		view->DrawString(fLine2, fLine2Start, &fDelta);
	}
}
