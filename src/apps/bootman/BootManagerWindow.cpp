/*
 * Copyright 2008, Michael Pfeiffer, laplace@users.sourceforge.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "BootManagerWindow.h"

#include "WizardView.h"

#include "EntryPage.h"
#include "PartitionsPage.h"
#include "DefaultPartitionPage.h"

#include <Application.h>
#include <Screen.h>

#include <math.h>

BootManagerWindow::BootManagerWindow()
	: BWindow(BRect(100, 100, 500, 400), "Boot Manager", B_TITLED_WINDOW, 
		B_ASYNCHRONOUS_CONTROLS)
	, fWindowCentered(false)
{
	float minWidth, maxWidth, minHeight, maxHeight;
	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	SetSizeLimits(250, maxWidth, 250, maxHeight);
	
	fWizardView = new WizardView(Bounds(), "wizard", B_FOLLOW_ALL);
	AddChild(fWizardView);
	
	fController.Initialize(fWizardView);
	
	AddShortcut('A', B_COMMAND_KEY, new BMessage(B_ABOUT_REQUESTED));
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


void
BootManagerWindow::WindowActivated(bool active)
{
	_CenterWindow();
}


void
BootManagerWindow::_CenterWindow()
{
	if (fWindowCentered)
		return;
	fWindowCentered = true;
	
	BScreen screen(this);
	if (!screen.IsValid())
		return;
		
	BRect frame = screen.Frame();
	BRect windowFrame = Frame();
	
	float left = floor((frame.Width() - windowFrame.Width()) / 2);
	float top = floor((frame.Height() - windowFrame.Height()) / 2);
	
	if (left < 20)
		left = 20;
	if (top < 20)
		top = 20;
	
	MoveTo(left, top);	
}

