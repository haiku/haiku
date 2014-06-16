/*
 * Copyright (c) 1997 by Massimino Pascal <Pascal.Massimon@ens.fr>
 * Copyright 2006-2014, Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		Massimino Pascal, Pascal.Massimon@ens.fr
 *		John Scipione, jscipione@gmail.com
 */
#include <math.h>
#include <stdio.h>

#include <Catalog.h>
#include <CheckBox.h>
#include <Slider.h>
#include <TextView.h>

#include "IFSSaver.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Screensaver IFS"


static const uint32 kMsgToggleAdditive		= 'tgad';
static const uint32 kMsgSetSpeed			= 'stsp';


//	#pragma mark - Instantiation function


extern "C" _EXPORT BScreenSaver*
instantiate_screen_saver(BMessage *message, image_id image)
{
	return new IFSSaver(message, image);
}


//	#pragma mark - IFSSaver


IFSSaver::IFSSaver(BMessage* message, image_id id)
	:
	BScreenSaver(message, id),
	BHandler("IFS Saver"),
	fIFS(NULL),
	fIsPreview(false),
	fLastDrawnFrame(0),
	fAdditive(false),
	fSpeed(6)
{
	fDirectInfo.bits = NULL;
	fDirectInfo.bytesPerRow = 0;

	if (message == NULL)
		return;

	if (message->FindBool("IFS additive", &fAdditive) != B_OK)
		fAdditive = false;
	if (message->FindInt32("IFS speed", &fSpeed) != B_OK)
		fSpeed = 6;
}


IFSSaver::~IFSSaver()
{
	if (Looper() != NULL)
		Looper()->RemoveHandler(this);

	_Cleanup();
}


void
IFSSaver::StartConfig(BView* view)
{
	BRect bounds = view->Bounds();
	bounds.InsetBy(10.0f, 10.0f);
	BRect frame(0.0f, 0.0f, bounds.Width(), 20.0f);

	// the additive check box
	fAdditiveCB = new BCheckBox(frame, "additive setting",
		B_TRANSLATE("Render dots additive"), new BMessage(kMsgToggleAdditive),
		B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);

	fAdditiveCB->SetValue(fAdditive);

	fAdditiveCB->ResizeToPreferred();
	bounds.bottom -= fAdditiveCB->Bounds().Height() * 2.0f;
	fAdditiveCB->MoveTo(bounds.LeftBottom());

	view->AddChild(fAdditiveCB);

	// the additive check box
	fSpeedS = new BSlider(frame, "speed setting",
		B_TRANSLATE("Morphing speed:"), new BMessage(kMsgSetSpeed), 1, 12,
		B_BLOCK_THUMB, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);

	fSpeedS->SetValue(fSpeed);
	fSpeedS->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fSpeedS->SetHashMarkCount(12);

	fSpeedS->ResizeToPreferred();
	bounds.bottom -= fSpeedS->Bounds().Height() + 15.0f;
	fSpeedS->MoveTo(bounds.LeftBottom());

	view->AddChild(fSpeedS);

	// the info text view
	BRect textRect = bounds;
	textRect.OffsetTo(0.0, 0.0);
	BTextView* textView = new BTextView(bounds, B_EMPTY_STRING, textRect,
		B_FOLLOW_ALL, B_WILL_DRAW);
	textView->SetViewColor(view->ViewColor());

	BString aboutScreenSaver(B_TRANSLATE("%screenSaverName%\n\n"
		B_UTF8_COPYRIGHT " 1997 Massimino Pascal\n\n"
		"xscreensaver port by Stephan Aßmus\n"
		"<stippi@yellowbites.com>"));
	BString screenSaverName(B_TRANSLATE("Iterated Function System"));

	aboutScreenSaver.ReplaceFirst("%screenSaverName%", screenSaverName);
	textView->Insert(aboutScreenSaver);

	textView->SetStylable(true);
	textView->SetFontAndColor(0, screenSaverName.Length(), be_bold_font);

	textView->MakeEditable(false);

	view->AddChild(textView);

	// make sure we receive messages from the views we added
	if (BWindow* window = view->Window())
		window->AddHandler(this);

	fAdditiveCB->SetTarget(this);
	fSpeedS->SetTarget(this);
}


status_t
IFSSaver::StartSaver(BView* view, bool preview)
{
	display_mode mode;
	BScreen screen(B_MAIN_SCREEN_ID);
	screen.GetMode(&mode);
	float totalSize = mode.timing.h_total * mode.timing.v_total;
	float fps = mode.timing.pixel_clock * 1000.0f / totalSize;

	SetTickSize((bigtime_t)floor(1000000.0 / fps + 0.5));

	fIsPreview = preview;

	if (view == NULL)
		return B_BAD_VALUE;

	_Init(view->Bounds());
	if (fIFS == NULL)
		return B_ERROR;

	fIFS->SetAdditive(fAdditive || fIsPreview);
	fIFS->SetSpeed(fSpeed);

	return B_OK;
}


void
IFSSaver::StopSaver()
{
	_Cleanup();
}


void
IFSSaver::DirectConnected(direct_buffer_info* info)
{
	int32 request = info->buffer_state & B_DIRECT_MODE_MASK;

	switch (request) {
		case B_DIRECT_START:
			fDirectInfo.bits = info->bits;
			fDirectInfo.bytesPerRow = info->bytes_per_row;
			fDirectInfo.bits_per_pixel = info->bits_per_pixel;
			fDirectInfo.format = info->pixel_format;
			fDirectInfo.bounds = info->window_bounds;
			break;

		case B_DIRECT_STOP:
			fDirectInfo.bits = NULL;
			break;
	}
}


void
IFSSaver::Draw(BView* view, int32 frame)
{
	if (frame == 0) {
		fLastDrawnFrame = -1;
		view->SetHighColor(0, 0, 0);
		view->FillRect(view->Bounds());
	}

	int32 frames = frame - fLastDrawnFrame;
	if ((fIsPreview || fDirectInfo.bits == NULL) && fLocker.Lock()) {
		fIFS->Draw(view, NULL, frames);

		fLastDrawnFrame = frame;
		fLocker.Unlock();
	}
}


void
IFSSaver::DirectDraw(int32 frame)
{
	if (frame == 0)
		fLastDrawnFrame = -1;

	if (!fIsPreview && fDirectInfo.bits != NULL) {
		fIFS->Draw(NULL, &fDirectInfo, frame - fLastDrawnFrame);
		fLastDrawnFrame = frame;
	}
}


status_t
IFSSaver::SaveState(BMessage* into) const
{
	status_t ret = B_BAD_VALUE;
	if (into != NULL) {
		ret = into->AddBool("IFS additive", fAdditive);
		if (ret >= B_OK)
			ret = into->AddInt32("IFS speed", fSpeed);
	}

	return ret;
}


void
IFSSaver::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgToggleAdditive:
			if (fLocker.Lock() && fIFS != NULL) {
				fAdditive = fAdditiveCB->Value() == B_CONTROL_ON;
				fIFS->SetAdditive(fAdditive || fIsPreview);
				fLocker.Unlock();
			}
			break;

		case kMsgSetSpeed:
			if (fLocker.Lock() && fIFS != NULL) {
				fSpeed = fSpeedS->Value();
				fIFS->SetSpeed(fSpeed);
				fLocker.Unlock();
			}
			break;

		default:
			BHandler::MessageReceived(message);
	}
}


void
IFSSaver::_Init(BRect bounds)
{
	if (fLocker.Lock()) {
		delete fIFS;
		fIFS = new IFS(bounds);
		fLocker.Unlock();
	}
}


void
IFSSaver::_Cleanup()
{
	if (fLocker.Lock()) {
		delete fIFS;
		fIFS = NULL;
		fLocker.Unlock();
	}
}
