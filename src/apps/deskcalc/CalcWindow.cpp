/*
 * Copyright 2006-2012 Haiku, Inc. All Rights Reserved.
 * Copyright 1997, 1998 R3 Software Ltd. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		John Scipione, jscipione@gmail.com
 *		Timothy Wayper, timmy@wunderbear.com
 */


#include "CalcWindow.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <Application.h>
#include <Catalog.h>
#include <Dragger.h>
#include <Screen.h>

#include "CalcApplication.h"
#include "CalcOptions.h"
#include "CalcView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Window"


CalcWindow::CalcWindow(BRect frame, BMessage* settings)
	:
	BWindow(frame, kAppName, B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS
		| B_NOT_ANCHORED_ON_ACTIVATE)
{
	// create calculator view with calculator description and
	// desktop background color
	BScreen screen(this);
	rgb_color baseColor = screen.DesktopColor();

	// Size Limits are defined in CalcView.h
	SetSizeLimits(kMinimumWidthBasic, kMaximumWidthBasic,
		kMinimumHeightBasic, kMaximumHeightBasic);

	frame.OffsetTo(B_ORIGIN);
	fCalcView = new CalcView(Frame(), baseColor, settings);

	// create replicant dragger
	BRect replicantFrame(frame);
	replicantFrame.top = replicantFrame.bottom - 7.0f;
	replicantFrame.left = replicantFrame.right - 7.0f;
	BDragger* dragger = new BDragger(replicantFrame, fCalcView,
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);

	// attach views
	AddChild(fCalcView);
	fCalcView->AddChild(dragger);

	BRect rect;
	if (settings->FindRect("window frame", &rect) == B_OK)
		SetFrame(rect);
	else
		SetFrame(frame, true);

	// Add shortcut keys to menu options
	AddShortcut('0', B_COMMAND_KEY,
		new BMessage(MSG_OPTIONS_KEYPAD_MODE_COMPACT));
	AddShortcut('1', B_COMMAND_KEY,
		new BMessage(MSG_OPTIONS_KEYPAD_MODE_BASIC));
	AddShortcut('2', B_COMMAND_KEY,
		new BMessage(MSG_OPTIONS_KEYPAD_MODE_SCIENTIFIC));
}


CalcWindow::~CalcWindow()
{
}


void
CalcWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_OPTIONS_AUTO_NUM_LOCK:
			fCalcView->ToggleAutoNumlock();
			break;

		case MSG_OPTIONS_ANGLE_MODE_RADIAN:
			fCalcView->SetDegreeMode(false);
			return;

		case MSG_OPTIONS_ANGLE_MODE_DEGREE:
			fCalcView->SetDegreeMode(true);
			return;

		case MSG_OPTIONS_KEYPAD_MODE_COMPACT:
			fCalcView->SetKeypadMode(KEYPAD_MODE_COMPACT);
			break;

		case MSG_OPTIONS_KEYPAD_MODE_BASIC:
			fCalcView->SetKeypadMode(KEYPAD_MODE_BASIC);
			break;

		case MSG_OPTIONS_KEYPAD_MODE_SCIENTIFIC:
			fCalcView->SetKeypadMode(KEYPAD_MODE_SCIENTIFIC);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
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
