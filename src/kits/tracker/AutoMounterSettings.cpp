/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

#include <Button.h>
#include <Debug.h>
#include <Message.h>
#include <RadioButton.h>
#include <Window.h>

#include "AutoMounter.h"
#include "AutoMounterSettings.h"

const uint32 kDone = 'done';
const uint32 kMountAllNow = 'done';
const uint32 kAutomountSettingsChanged = 'achg';
const uint32 kBootMountSettingsChanged = 'bchg';
const uint32 kAutoAll = 'aall';
const uint32 kAutoBFS = 'abfs';
const uint32 kAutoHFS = 'ahfs';
const uint32 kInitAll = 'iall';
const uint32 kInitBFS = 'ibfs';
const uint32 kInitHFS = 'ihfs';

const BPoint kButtonSize(80, 20);
const BPoint kSmallButtonSize(60, 20);
const rgb_color kLightGray = { 216, 216, 216, 255};

const int32 kCheckBoxSpacing = 20;

AutomountSettingsDialog *AutomountSettingsDialog::oneCopyOnly = NULL;

void 
AutomountSettingsDialog::RunAutomountSettings(AutoMounter *target)
{
	// either activate an existing mount settings dialog or create a new one
	if (oneCopyOnly) {
		oneCopyOnly->Activate();
		return;
	}

	BMessage message;
	target->GetSettings(&message);
	(new AutomountSettingsDialog(&message, target))->Show();
}

AutomountSettingsPanel::AutomountSettingsPanel(BRect frame, 
	BMessage *settings, AutoMounter *target)
	:	BBox(frame, "", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS
			| B_NAVIGABLE_JUMP, B_PLAIN_BORDER),
		fTarget(target)
{
	SetViewColor(kLightGray);

	BRect checkBoxRect(Bounds());

	BRect boxRect(Bounds());
	boxRect.InsetBy(10, 15);
	boxRect.bottom = boxRect.top + 85;
	BBox *box = new BBox(boxRect, "autoMountBox", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_PULSE_NEEDED | B_NAVIGABLE_JUMP);
	box->SetLabel("Automatic Disk Mounting:");
	AddChild(box);

	checkBoxRect = box->Bounds();
	checkBoxRect.InsetBy(10, 18);
	
	checkBoxRect.bottom = checkBoxRect.top + 20;

	scanningDisabledCheck = new BRadioButton(checkBoxRect, "scanningOff",
		"Don't Automount", new BMessage(kAutomountSettingsChanged));
	box->AddChild(scanningDisabledCheck);

	checkBoxRect.OffsetBy(0, kCheckBoxSpacing);
	autoMountAllBFSCheck = new BRadioButton(checkBoxRect, "autoBFS",
		"All BeOS Disks", new BMessage(kAutomountSettingsChanged));
	box->AddChild(autoMountAllBFSCheck);

	checkBoxRect.OffsetBy(0, kCheckBoxSpacing);
	autoMountAllCheck = new BRadioButton(checkBoxRect, "autoAll",
		"All Disks", new BMessage(kAutomountSettingsChanged));
	box->AddChild(autoMountAllCheck);
	

	boxRect.OffsetTo(boxRect.left, boxRect.bottom + 15);
	boxRect.bottom = boxRect.top + 105;
	box = new BBox(boxRect, "", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS
			| B_PULSE_NEEDED | B_NAVIGABLE_JUMP);
	box->SetLabel("Disk Mounting During Boot:");
	AddChild(box);

	checkBoxRect = box->Bounds();
	checkBoxRect.InsetBy(10, 18);
	
	checkBoxRect.bottom = checkBoxRect.top + 20;
	initialDontMountCheck = new BRadioButton(checkBoxRect, "initialNone",
		"Only The Boot Disk", new BMessage(kBootMountSettingsChanged));
	box->AddChild(initialDontMountCheck);

	checkBoxRect.OffsetBy(0, kCheckBoxSpacing);
	initialMountRestoreCheck = new BRadioButton(checkBoxRect, "initialRestore",
		"Previously Mounted Disks", new BMessage(kBootMountSettingsChanged));
	box->AddChild(initialMountRestoreCheck);
	
	checkBoxRect.OffsetBy(0, kCheckBoxSpacing);
	initialMountAllBFSCheck = new BRadioButton(checkBoxRect, "initialBFS",
		"All BeOS Disks", new BMessage(kBootMountSettingsChanged));
	box->AddChild(initialMountAllBFSCheck);
	
	checkBoxRect.OffsetBy(0, kCheckBoxSpacing);
	initialMountAllCheck = new BRadioButton(checkBoxRect, "initialAll",
		"All Disks", new BMessage(kBootMountSettingsChanged));
	box->AddChild(initialMountAllCheck);

	
	BRect buttonRect(Bounds());
	buttonRect.InsetBy(15, 15);
	buttonRect.SetLeftTop(buttonRect.RightBottom() - kSmallButtonSize);
	fDone = new BButton(buttonRect, "done", "Done", new BMessage(kDone));

	buttonRect.OffsetTo(buttonRect.left - 15 - buttonRect.Width(), buttonRect.top);
	buttonRect.left = buttonRect.left - 60;
	fMountAllNow = new BButton(buttonRect, "mountAll", "Mount all disks now",
		new BMessage(kMountAllNow));

	AddChild(fMountAllNow);

	AddChild(fDone);
	fDone->MakeDefault(true);

	bool result;
	if (settings->FindBool("autoMountAll", &result) == B_OK && result)
		autoMountAllCheck->SetValue(1);
	else if (settings->FindBool("autoMountAllBFS", &result) == B_OK && result)
		autoMountAllBFSCheck->SetValue(1);
	else
		scanningDisabledCheck->SetValue(1);

	if (settings->FindBool("suspended", &result) == B_OK && result)
		scanningDisabledCheck->SetValue(1);
		
	if (settings->FindBool("initialMountAll", &result) == B_OK && result)
		initialMountAllCheck->SetValue(1);
	else if (settings->FindBool("initialMountRestore", &result) == B_OK && result)
		initialMountRestoreCheck->SetValue(1);
	else if (settings->FindBool("initialMountAllBFS", &result) == B_OK && result)
		initialMountAllBFSCheck->SetValue(1);
	else
		initialDontMountCheck->SetValue(1);
	
}

AutomountSettingsPanel::~AutomountSettingsPanel()
{
}

void
AutomountSettingsPanel::SendSettings(bool rescan)
{
	BMessage message(kSetAutomounterParams);
	
	message.AddBool("autoMountAll", (bool)autoMountAllCheck->Value());
	message.AddBool("autoMountAllBFS", (bool)autoMountAllBFSCheck->Value());
	if (autoMountAllBFSCheck->Value())
		message.AddBool("autoMountAllHFS", false);

	message.AddBool("suspended", (bool)scanningDisabledCheck->Value());
	message.AddBool("rescanNow", rescan);

	message.AddBool("initialMountAll", (bool)initialMountAllCheck->Value());
	message.AddBool("initialMountAllBFS", (bool)initialMountAllBFSCheck->Value());
	message.AddBool("initialMountRestore", (bool)initialMountRestoreCheck->Value());
	if (initialDontMountCheck->Value()) 
		message.AddBool("initialMountAllHFS", false);
	
	fTarget->PostMessage(&message, NULL);
}

void
AutomountSettingsPanel::AttachedToWindow()
{
	initialMountAllCheck->SetTarget(this);	
	initialMountAllBFSCheck->SetTarget(this);	
	initialMountRestoreCheck->SetTarget(this);	
	initialDontMountCheck->SetTarget(this);	
	autoMountAllCheck->SetTarget(this);	
	autoMountAllBFSCheck->SetTarget(this);	
	scanningDisabledCheck->SetTarget(this);	
	fDone->SetTarget(this);	
	fMountAllNow->SetTarget(fTarget);
}

void
AutomountSettingsPanel::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kDone:
		case B_QUIT_REQUESTED:
			Window()->Quit();
			break;

		case kAutomountSettingsChanged:
			SendSettings(true);
			break;

		case kBootMountSettingsChanged:
			SendSettings(false);
			break;

		default:
			_inherited::MessageReceived(message);
			break;
	}
}

AutomountSettingsDialog::AutomountSettingsDialog(BMessage *settings,  
	AutoMounter *target)
	:	BWindow(BRect(100, 100, 320, 370), "Disk Mount Settings", 
			B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	AddChild(new AutomountSettingsPanel(Bounds(), settings, target));
	ASSERT(!oneCopyOnly);
	oneCopyOnly = this;
}

AutomountSettingsDialog::~AutomountSettingsDialog()
{
	ASSERT(oneCopyOnly);
	oneCopyOnly = NULL;
}

