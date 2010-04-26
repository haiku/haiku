/*
 * Copyright 2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 * 
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */


#include "BootManagerWindow.h"

#include "WizardView.h"

#include "EntryPage.h"
#include "PartitionsPage.h"
#include "DefaultPartitionPage.h"

#include <Application.h>
#include <Catalog.h>
#include <Roster.h>
#include <Screen.h>

#include <math.h>

#include "tracker_private.h"


#undef TR_CONTEXT
#define TR_CONTEXT "BootManagerWindow"



BootManagerWindow::BootManagerWindow()
	:
	BWindow(BRect(100, 100, 500, 400), TR_CMT("Boot Manager", "Window Title"),
		B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE)
{
	float minWidth, maxWidth, minHeight, maxHeight;
	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	SetSizeLimits(250, maxWidth, 250, maxHeight);
	
	fWizardView = new WizardView(Bounds(), "wizard", B_FOLLOW_ALL);
	AddChild(fWizardView);
	
	fController.Initialize(fWizardView);
	
	AddShortcut('A', B_COMMAND_KEY, new BMessage(B_ABOUT_REQUESTED));

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
		
		case B_ABOUT_REQUESTED:
			be_app_messenger.SendMessage(B_ABOUT_REQUESTED);
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

