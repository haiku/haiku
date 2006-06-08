/*
 * Copyright 2006 Haiku, Inc. All Rights Reserved.
 * Copyright 1997, 1998 R3 Software Ltd. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Timothy Wayper <timmy@wunderbear.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "CalcWindow.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <Application.h>
#include <Dragger.h>
#include <Screen.h>

#include "CalcView.h"


static const char* kWindowTitle		= "Desk Calculator";


CalcWindow::CalcWindow(BRect frame)
	: BWindow(frame, kWindowTitle, B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	// create calculator view with calculator description and
	// desktop background color
	BScreen screen(this);
	rgb_color baseColor = screen.DesktopColor();

	frame.OffsetTo(B_ORIGIN);
	fCalcView = new CalcView(frame, baseColor);
	
	// create replicant dragger
	frame.top = frame.bottom - 7.0f;
	frame.left = frame.right - 7.0f;
	BDragger* dragger = new BDragger(frame, fCalcView,
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	
	// attach views
	AddChild(fCalcView);
	fCalcView->AddChild(dragger);

	SetSizeLimits(50.0, 400.0, 22.0, 400.0);
}


CalcWindow::~CalcWindow()
{
}


void
CalcWindow::Show()
{
	// NOTE: done here because the CalcView
	// turns on numlock if the options say so...
	// so we want to call MakeFocus() after
	// the options have been read for sure...
	fCalcView->MakeFocus();
	BWindow::Show();
}


bool
CalcWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	Hide();

	// NOTE: don't quit, since the app needs us
	// for saving settings yet...
	return false;
}


bool
CalcWindow::LoadSettings(BMessage* archive)
{
	status_t ret = fCalcView->LoadSettings(archive);

	if (ret < B_OK)
		return false;

	BRect frame;
	ret = archive->FindRect("window frame", &frame);
	if (ret < B_OK)
		return false;

	SetFrame(frame);

	return true;
}


status_t
CalcWindow::SaveSettings(BMessage* archive) const
{
	status_t ret = archive->AddRect("window frame", Frame());
	if (ret < B_OK)
		return ret;

	return fCalcView->SaveSettings(archive);
}


void
CalcWindow::SetFrame(BRect frame, bool forceCenter)
{
	// make sure window frame is on screen (center, if not)
	BScreen screen(this);
	BRect screenFrame = screen.Frame();
	if (forceCenter || !screenFrame.Contains(frame)) {
		float left = (screenFrame.Width() - frame.Width()) / 2.0;
		float top = (screenFrame.Height() - frame.Height()) / 2.0;
		left += screenFrame.left;
		top += screenFrame.top;
		frame.OffsetTo(left, top);
	}

	MoveTo(frame.left, frame.top);
	ResizeTo(frame.Width(), frame.Height());
}
