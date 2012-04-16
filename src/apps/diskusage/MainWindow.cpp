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
#include <Catalog.h>
#include <Node.h>
#include <Roster.h>
#include <Screen.h>

#include <LayoutBuilder.h>

#include "DiskUsage.h"
#include "ControlsView.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainWindow"

MainWindow::MainWindow(BRect pieRect)
	:
	BWindow(pieRect, B_TRANSLATE_SYSTEM_NAME("DiskUsage"), B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE
		| B_AUTO_UPDATE_SIZE_LIMITS)
{
	fControlsView = new ControlsView();

	SetLayout(new BGroupLayout(B_VERTICAL));

	AddChild(BLayoutBuilder::Group<>(B_VERTICAL)
		.Add(fControlsView)
		.SetInsets(5, 5, 5, 5)
	);
	float maxHeight = BScreen(this).Frame().Height() - 12;
	fControlsView->SetExplicitMaxSize(BSize(maxHeight, maxHeight));
}


MainWindow::~MainWindow()
{
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kBtnRescan:
			fControlsView->MessageReceived(message);
			break;
		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
			fControlsView->MessageReceived(message);
			break;

		case kBtnHelp:
			if (helpFileWasFound)
				be_roster->Launch(&helpFileRef);
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
MainWindow::SetRescanEnabled(bool enabled)
{
	fControlsView->SetRescanEnabled(enabled);
}


void
MainWindow::ShowInfo(const FileInfo* info)
{
	fControlsView->ShowInfo(info);
}


BVolume*
MainWindow::FindDeviceFor(dev_t device, bool invoke)
{
	return fControlsView->FindDeviceFor(device, invoke);
}
