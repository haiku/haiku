/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>.
 * Copyright (c) 2009 Philippe Saint-Pierre, stpere@gmail.com
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 * Copyright (c) 1999 Mike Steed. You are free to use and distribute this software
 * as long as it is accompanied by it's documentation and this copyright notice.
 * The software comes with no warranty, etc.
 */


#include "MainWindow.h"

#include <Application.h>
#include <Node.h>
#include <Roster.h>
#include <Screen.h>

#include "Common.h"
#include "ControlsView.h"
#include "PieView.h"
#include "StatusView.h"


const float kMinWinSize = 275.0;


MainWindow::MainWindow(BRect pieRect)
	:
	BWindow(pieRect, "DiskUsage", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE)
{
	BRect r = Bounds();
	r.bottom = r.top + 1.0;
	fControlsView = new ControlsView(r);
	float cvHeight = fControlsView->Bounds().Height();
	AddChild(fControlsView);

	r = Bounds();
	r.top = r.bottom - 1.0;
	fStatusView = new StatusView(r);
	float svHeight = fStatusView->Bounds().Height();

	float winHeight = pieRect.Height() + cvHeight + svHeight;
	fStatusView->MoveTo(0.0, winHeight - svHeight);
	ResizeTo(r.Width(), winHeight);

	AddChild(fStatusView);

	r = fControlsView->Frame();
	r.top = r.bottom + 1.0;
	r.bottom = fStatusView->Frame().top - 1.0;
	fPieView = new PieView(r, this);
	AddChild(fPieView);

	Show();

	// Note: The following code is semi-broken because BScreen::Frame()
	// returns incorrect dimensions for the G200 in 1152x864 mode.  I reported
	// this bug, and Be said it's not a bug -- the Matrox driver actually uses
	// a resolution of 1152x900 in that mode.  Oh well.
	Lock();
	float extraHeight = fControlsView->Bounds().Height()
		+ fStatusView->Bounds().Height();
	float minHeight = kMinWinSize + extraHeight;
	float maxHeight = BScreen(this).Frame().Height();
	float maxWidth = maxHeight - extraHeight;
	Unlock();

	SetSizeLimits(kMinWinSize, maxWidth, minHeight, maxHeight);
}


MainWindow::~MainWindow()
{
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kBtnRescan:
		case kMenuSelectVol:
		case B_REFS_RECEIVED:
			fPieView->MessageReceived(message);
			break;

		case kBtnHelp:
			be_roster->Launch(&kHelpFileRef);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
MainWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


// #pragma mark -


void
MainWindow::ShowInfo(const FileInfo* info)
{
	fStatusView->ShowInfo(info);
}


void
MainWindow::SetRescanEnabled(bool enabled)
{
	fControlsView->SetRescanEnabled(enabled);
}


BVolume*
MainWindow::FindDeviceFor(dev_t device, bool invoke)
{
	return fControlsView->FindDeviceFor(device, invoke);
}
