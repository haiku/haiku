/*
 * Copyright 2007-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ryan Leavengood, leavengood@gmail.com
 */


#include <Catalog.h>
#include <Font.h>
#include <ScreenSaver.h>
#include <StringView.h>
#include <View.h>

#include <BuildScreenSaverDefaultSettingsView.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Screensaver DebugNow"


const rgb_color kMediumBlue = {0, 0, 100};
const rgb_color kWhite = {255, 255, 255};


// Inspired by the classic BeOS BuyNow screensaver, of course
class DebugNow : public BScreenSaver
{
	public:
							DebugNow(BMessage *archive, image_id);
		void				Draw(BView *view, int32 frame);
		void				StartConfig(BView *view);
		status_t			StartSaver(BView *view, bool preview);
		
	private:
		const char* 		fLine1;
		const char*			fLine2;
		BPoint				fLine1Start;
		BPoint				fLine2Start;
		escapement_delta	fDelta;
};


BScreenSaver* instantiate_screen_saver(BMessage *msg, image_id image) 
{ 
	return new DebugNow(msg, image);
} 


DebugNow::DebugNow(BMessage *archive, image_id id)
	:	
	BScreenSaver(archive, id),
	fLine1("DEBUG"),
	fLine2("NOW")
{
}


void 
DebugNow::StartConfig(BView *view) 
{
	BPrivate::BuildScreenSaverDefaultSettingsView(view, "DEBUG NOW",
		B_TRANSLATE("by Ryan Leavengood"));
} 


status_t 
DebugNow::StartSaver(BView *view, bool preview)
{
	float viewWidth = view->Bounds().Width();
	float viewHeight = view->Bounds().Height();
	
	BFont font;
	view->GetFont(&font);
	font.SetSize(viewHeight / 2.5);
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
	
	// Set tick size to 500,000 microseconds = 0.5 second
	SetTickSize(500000);
	
	return B_OK;
}


void 
DebugNow::Draw(BView *view, int32 frame)
{
	// On first frame set the low color to make the text rendering correct
	if (frame == 0)
		view->SetLowColor(kMediumBlue);

	// Draw the background color every frame 
	view->SetHighColor(kMediumBlue);
	view->FillRect(view->Bounds());

	// Draw the text every other frame to make the it blink
	if (frame % 2 == 1) {
		view->SetHighColor(kWhite);
		view->DrawString(fLine1, fLine1Start, &fDelta);
		view->DrawString(fLine2, fLine2Start, &fDelta);
	}
}

