/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ryan Leavengood
 */


#include <ScreenSaver.h>
#include <View.h>
#include <StringView.h>
#include <Font.h>


const rgb_color kMediumBlue = {0, 0, 100};
const rgb_color kWhite = {255, 255, 255};


// Inspired by the classic BeOS screensaver, of course
class BuyNow : public BScreenSaver
{
	public:
					BuyNow(BMessage *archive, image_id);
		void		Draw(BView *view, int32 frame);
		void		StartConfig(BView *view);
		status_t	StartSaver(BView *view, bool preview);
		
	private:
		const char *fLine1;
		const char *fLine2;
		BPoint		fLine1Start;
		BPoint		fLine2Start;
};


BScreenSaver *instantiate_screen_saver(BMessage *msg, image_id image) 
{ 
	return new BuyNow(msg, image);
} 


BuyNow::BuyNow(BMessage *archive, image_id id)
 :	BScreenSaver(archive, id)
 ,	fLine1("BUY")
 ,	fLine2("NOW")
{
}


void 
BuyNow::StartConfig(BView *view) 
{ 
	view->AddChild(new BStringView(BRect(20, 10, 200, 35), "", "BUY NOW, by Ryan Leavengood"));
} 


status_t 
BuyNow::StartSaver(BView *view, bool preview)
{
	float width = view->Bounds().Width();
	float height = view->Bounds().Height();
	
	BFont font;
	view->GetFont(&font);
	font.SetSize(height / 2.5);
	view->SetFont(&font);
	
	BRect rect;
	escapement_delta delta;
	delta.nonspace = 0;
	delta.space = 0;
	// If anyone has suggestions for how to clean this up, speak up
	font.GetBoundingBoxesForStrings(&fLine1, 1, B_SCREEN_METRIC, &delta, &rect);
	float y = ((height - (rect.Height() * 2 + height / 10)) / 2) + rect.Height();
	fLine1Start.Set((width - rect.Width()) / 2, y);
	font.GetBoundingBoxesForStrings(&fLine2, 1, B_SCREEN_METRIC, &delta, &rect);
	fLine2Start.Set((width - rect.Width()) / 2, y + rect.Height() + height / 10);
	
	return B_OK;
}


void 
BuyNow::Draw(BView *view, int32 frame)
{
	if (frame == 0) { 
		// fill with blue on first frame
		view->SetLowColor(kMediumBlue); 
		view->FillRect(view->Bounds(), B_SOLID_LOW); 

		// Set tick size to 500,000 microseconds = 0.5 second
		SetTickSize(500000);
	} else {
		// Drawing the background color every other frame to make the text blink
		if (frame % 2 == 1)
			view->SetHighColor(kWhite);
		else
			view->SetHighColor(kMediumBlue);

		view->DrawString(fLine1, fLine1Start);
		view->DrawString(fLine2, fLine2Start);
	}
}

