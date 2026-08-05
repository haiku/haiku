/*
 * Copyright 2003-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Jérôme Duval
 *		François Revol
 *		Axel Dörfler, axeld@pinc-software.de.
 *		John Scipione, jscipione@gmail.com
 */


#include "VolumeWindow.h"

#include <Box.h>
#include <GroupLayout.h>
#include <MenuItem.h>
#include <MessageRunner.h>
#include <Screen.h>

#include "VolumeControl.h"


VolumeWindow::VolumeWindow(BRect frame)
	:
	BWindow(frame, "VolumeWindow", B_BORDERED_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_WILL_ACCEPT_FIRST_CLICK | B_AUTO_UPDATE_SIZE_LIMITS, 0),
	fUpdatedCount(0)
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));

	BGroupLayout* layout = new BGroupLayout(B_HORIZONTAL);
	layout->SetInsets(5, 5, 5, 5);

	BBox* box = new BBox("sliderbox");
	box->SetLayout(layout);
	box->SetBorder(B_PLAIN_BORDER);
	AddChild(box);

	fVolumeSlider = new VolumeControl();
	fVolumeSlider->SetModificationMessage(new BMessage(kMsgVolumeUpdate));
	box->AddChild(fVolumeSlider);

	fVolumeSlider->SetTarget(this);
	ResizeTo(300, 50);

	// Make sure it's not outside the screen.
	const int32 kMargin = 3;
	BRect windowRect = Frame();
	BRect screenFrame(BScreen(B_MAIN_SCREEN_ID).Frame());
	if (screenFrame.right < windowRect.right + kMargin)
		MoveBy(- kMargin - windowRect.right + screenFrame.right, 0);
	if (screenFrame.bottom < windowRect.bottom + kMargin)
		MoveBy(0, - kMargin - windowRect.bottom + screenFrame.bottom);
	if (screenFrame.left > windowRect.left - kMargin)
		MoveBy(kMargin + screenFrame.left - windowRect.left, 0);
	if (screenFrame.top > windowRect.top - kMargin)
		MoveBy(0, kMargin + screenFrame.top - windowRect.top);
}


VolumeWindow::~VolumeWindow()
{
}


void
VolumeWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgVolumeUpdate:
			fUpdatedCount++;
			break;

		case kMsgVolumeChanged:
			if (fUpdatedCount < 2) {
				// If the slider was set by click only, wait a bit until
				// closing the window to give some feedback that how volume
				// was changed, and that it is.
				BMessage quit(B_QUIT_REQUESTED);
				BMessageRunner::StartSending(this, &quit, 150000, 1);
			} else
				Quit();
			break;

		case B_QUIT_REQUESTED:
			Quit();
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}

