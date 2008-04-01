/*
 * Copyright 2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 * 
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */


#include "EntryPage.h"


#include <RadioButton.h>
#include <TextView.h>

#include <string.h>


EntryPage::EntryPage(BMessage* settings, BRect frame, const char* name)
	: WizardPageView(settings, frame, name, B_FOLLOW_ALL, 
		B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE)
{
	_BuildUI();
}


EntryPage::~EntryPage()
{
}


void
EntryPage::FrameResized(float width, float height)
{
	WizardPageView::FrameResized(width, height);
	_Layout();
}


void
EntryPage::PageCompleted()
{
	fSettings->ReplaceBool("install", fInstall->Value() != 0);
}

static const float kTextDistance = 10;

void
EntryPage::_BuildUI()
{
	BRect rect(Bounds());
	
	fInstall = new BRadioButton(rect, "install", 
		"",
		new BMessage('null'));
	AddChild(fInstall);
	fInstall->ResizeToPreferred();

	BRect textRect(rect);
	textRect.left = fInstall->Frame().right + kTextDistance;
		
	fInstallText = CreateDescription(textRect, "installText", 
		"Install Boot Menu\n\n"
		"Choose this option to install a boot menu, "
		"allowing you to select which operating "
		"system to boot when you turn on your "
		"computer.\n");
	MakeHeading(fInstallText);
	AddChild(fInstallText);
	
	fUninstall = new BRadioButton(rect, "uninstall", 
		"",
		new BMessage('null'));
	AddChild(fUninstall);
	fUninstall->ResizeToPreferred();
	
	fUninstallText = CreateDescription(textRect, "uninstallText",
		"Uninstall Boot Menu\n\n"
		"Choose this option to remove the boot menu "
		"previously installed by this program.\n"
	);
	MakeHeading(fUninstallText);
	AddChild(fUninstallText);

	bool install;
	fSettings->FindBool("install", &install);
	
	if (install)
		fInstall->SetValue(1);
	else
		fUninstall->SetValue(1);
	
	_Layout();
}


void
EntryPage::_Layout()
{
	LayoutDescriptionVertically(fInstallText);
	
	float left = fUninstall->Frame().left;
	float top = fInstallText->Frame().bottom;
	fUninstall->MoveTo(left, top);
	
	left = fUninstallText->Frame().left;
	fUninstallText->MoveTo(left, top);

	LayoutDescriptionVertically(fUninstallText);
}

