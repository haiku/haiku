/*
 * Copyright 2008-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */


#include "BootManagerWindow.h"

#include <Application.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Roster.h>
#include <Screen.h>

#include <tracker_private.h>

#include "DefaultPartitionPage.h"
#include "PartitionsPage.h"
#include "WizardView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "BootManagerWindow"


BootManagerWindow::BootManagerWindow()
	:
	BWindow(BRect(100, 100, 500, 400), B_TRANSLATE_SYSTEM_NAME("BootManager"),
		B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE
		| B_AUTO_UPDATE_SIZE_LIMITS)
{
	float minWidth, maxWidth, minHeight, maxHeight;
	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	SetSizeLimits(250, maxWidth, 250, maxHeight);

	fWizardView = new WizardView("wizard");
	BLayoutBuilder::Group<>(this)
		.Add(fWizardView);

	fController.Initialize(fWizardView);

	CenterOnScreen();

	// Prevent minimizing this window if the user would have no way to
	// get back to it. (For example when only the Installer runs.)
	if (!be_roster->IsRunning(kDeskbarSignature))
		SetFlags(Flags() | B_NOT_MINIMIZABLE);
}


BootManagerWindow::~BootManagerWindow()
{
}


void
BootManagerWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kMessageNext:
			fController.Next(fWizardView);
			break;

		case kMessagePrevious:
			fController.Previous(fWizardView);
			break;

		default:
			BWindow::MessageReceived(msg);
	}
}


bool
BootManagerWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

