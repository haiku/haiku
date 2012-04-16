// Copyright 1999, Be Incorporated. All Rights Reserved.
// Copyright 2000-2004, Jun Suzuki. All Rights Reserved.
// Copyright 2007, Stephan Aßmus. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.
#include "MediaEncoderWindow.h"

#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>
#include <View.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MediaConverter-EncoderWindow"


MediaEncoderWindow::MediaEncoderWindow(BRect frame, BView* view)
	: BWindow(frame, B_TRANSLATE("Encoder parameters"), B_DOCUMENT_WINDOW_LOOK,
		B_MODAL_APP_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	// create and acquire the quit semaphore
	fQuitSem = create_sem(0, "encoder_view");

	fView = view;
	AddChild(fView);
	ResizeTo(fView->Bounds().Width(), fView->Bounds().Height());
}


MediaEncoderWindow::~MediaEncoderWindow()
{
	// The view must continue to exist until conversion complete.
	RemoveChild(fView);
	delete_sem(fQuitSem);
}


void
MediaEncoderWindow::MessageReceived(BMessage *msg)
{
}


bool
MediaEncoderWindow::QuitRequested()
{
	return true;
}


void
MediaEncoderWindow::Go()
{
	this->Show();

	// wait until window is quit
	while (acquire_sem(fQuitSem) == B_INTERRUPTED)
		;
}

