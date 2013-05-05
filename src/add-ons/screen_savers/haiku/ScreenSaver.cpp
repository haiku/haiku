/* 
**
** A simple screensaver, displays the text "Haiku" at random locations.
**
** Version: 3.0
**
** Copyright (c) 2002, 2005 Marcus Overhagen. All Rights Reserved.
** This file may be used under the terms of the MIT License.
*/


#include <stdlib.h>

#include <Catalog.h>
#include <Font.h>
#include <ScreenSaver.h>
#include <StringView.h>
#include <View.h>

#include <BuildScreenSaverDefaultSettingsView.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Screensaver Haiku"


class ScreenSaver : public BScreenSaver
{
public:
				ScreenSaver(BMessage *archive, image_id);
	void		Draw(BView *view, int32 frame);
	void		StartConfig(BView *view);
	status_t	StartSaver(BView *view, bool preview);
	
private:
	const char *fText;
	float		fX;
	float		fY;
	float		fSizeX;
	float		fSizeY;
	float		fTextHeight;
	float		fTextWith;
	bool		fIsPreview;
};


BScreenSaver *instantiate_screen_saver(BMessage *msg, image_id image) 
{ 
	return new ScreenSaver(msg, image);
} 


ScreenSaver::ScreenSaver(BMessage *archive, image_id id)
 :	BScreenSaver(archive, id)
 ,	fText("Haiku")
 ,	fX(0)
 ,	fY(0)
{
}


void 
ScreenSaver::StartConfig(BView *view) 
{ 
	BPrivate::BuildScreenSaverDefaultSettingsView(view, "Haiku",
		B_TRANSLATE("by Marcus Overhagen"));
} 


status_t 
ScreenSaver::StartSaver(BView *view, bool preview)
{
	// save view dimensions and preview mode
	fIsPreview = preview;
	fSizeX = view->Bounds().Width();
	fSizeY = view->Bounds().Height();
	
	// set a new font, about 1/8th of view height, and bold
	BFont font;
	view->GetFont(&font);
	font.SetSize(fSizeY / 8);
	font.SetFace(B_BOLD_FACE);
	view->SetFont(&font);
	
	// find out space needed for text display
	BRect rect;
	escapement_delta delta;
	delta.nonspace = 0;
	delta.space = 0;
	font.GetBoundingBoxesForStrings(&fText, 1, B_SCREEN_METRIC, &delta, &rect);
	fTextHeight = rect.Height();
	fTextWith = rect.Width();
	
	// seed the random number generator
	srand((int)system_time());
	
	return B_OK;
}


void 
ScreenSaver::Draw(BView *view, int32 frame)
{
	if (frame == 0) { 
		// fill with black on first frame
		view->SetLowColor(0, 0, 0); 
		view->FillRect(view->Bounds(), B_SOLID_LOW); 
	} else {
		// erase old text on all other frames
		view->SetHighColor(0, 0, 0);
		view->DrawString(fText, BPoint(fX, fY));
	}

	// find some new text coordinates
	fX = rand() % int(fSizeX - fTextWith);
	fY = rand() % int(fSizeY - fTextHeight - (fIsPreview ? 2 : 20)) + fTextHeight;

	// draw new text
	view->SetHighColor(0, 255, 0);
	view->DrawString(fText, BPoint(fX, fY));

	// randomize time until next update (preview mode is faster)
	SetTickSize(((rand() % 4) + 1) * (fIsPreview ? 300000 : 1000000));
}
