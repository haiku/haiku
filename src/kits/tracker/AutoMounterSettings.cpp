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


#include "AutoMounter.h"
#include "AutoMounterSettings.h"

#include <Box.h>
#include <Button.h>
#include <Debug.h>
#include <Message.h>
#include <RadioButton.h>
#include <Window.h>


const uint32 kAutomountSettingsChanged = 'achg';
const uint32 kBootMountSettingsChanged = 'bchg';
const uint32 kAutoAll = 'aall';
const uint32 kAutoBFS = 'abfs';
const uint32 kAutoHFS = 'ahfs';
const uint32 kInitAll = 'iall';
const uint32 kInitBFS = 'ibfs';
const uint32 kInitHFS = 'ihfs';

class AutomountSettingsPanel : public BBox {
	public:
		AutomountSettingsPanel(BRect, BMessage *, AutoMounter *);
		virtual ~AutomountSettingsPanel();

	protected:
		virtual void MessageReceived(BMessage *);
		virtual void AttachedToWindow();
		virtual void GetPreferredSize(float* _width, float* _height);
	
	private:
		void SendSettings(bool rescan);
	
		BRadioButton *fInitialDontMountCheck;
		BRadioButton *fInitialMountAllBFSCheck;
		BRadioButton *fInitialMountAllCheck;
		BRadioButton *fInitialMountRestoreCheck;

		BRadioButton *fScanningDisabledCheck;
		BRadioButton *fAutoMountAllBFSCheck;
		BRadioButton *fAutoMountAllCheck;

		BButton *fDone;
		BButton *fMountAllNow;

		AutoMounter *fTarget;

		typedef BBox _inherited;
};

AutomountSettingsDialog *AutomountSettingsDialog::sOneCopyOnly = NULL;


AutomountSettingsPanel::AutomountSettingsPanel(BRect frame, 
		BMessage *settings, AutoMounter *target)
	: BBox(frame, "", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS
			| B_NAVIGABLE_JUMP, B_NO_BORDER),
		fTarget(target)
{
	// "Automatic Disk Mounting" group

	BRect boxRect(Bounds());
	boxRect.InsetBy(8.0f, 8.0f);
	boxRect.bottom = boxRect.top + 85;
	BBox *box = new BBox(boxRect, "autoMountBox", B_FOLLOW_LEFT_RIGHT,
		B_WILL_DRAW | B_FRAME_EVENTS | B_PULSE_NEEDED | B_NAVIGABLE_JUMP);
	box->SetLabel("Automatic Disk Mounting");
	AddChild(box);

	font_height fontHeight;
	box->GetFontHeight(&fontHeight);

	BRect checkBoxRect(box->Bounds());
	checkBoxRect.InsetBy(8.0f, 8.0f);
	checkBoxRect.top += fontHeight.ascent + fontHeight.descent;

	fScanningDisabledCheck = new BRadioButton(checkBoxRect, "scanningOff",
		"Don't Automount", new BMessage(kAutomountSettingsChanged));
	fScanningDisabledCheck->ResizeToPreferred();
	box->AddChild(fScanningDisabledCheck);

	checkBoxRect.OffsetBy(0, fScanningDisabledCheck->Bounds().Height());
	fAutoMountAllBFSCheck = new BRadioButton(checkBoxRect, "autoBFS",
		"All BeOS Disks", new BMessage(kAutomountSettingsChanged));
	fAutoMountAllBFSCheck->ResizeToPreferred();
	box->AddChild(fAutoMountAllBFSCheck);

	checkBoxRect.OffsetBy(0, fScanningDisabledCheck->Bounds().Height());
	fAutoMountAllCheck = new BRadioButton(checkBoxRect, "autoAll",
		"All Disks", new BMessage(kAutomountSettingsChanged));
	fAutoMountAllCheck->ResizeToPreferred();
	box->AddChild(fAutoMountAllCheck);

	box->ResizeTo(box->Bounds().Width(), fAutoMountAllCheck->Frame().bottom + 8.0f);

	// "Disk Mounting During Boot" group

	boxRect.top = box->Frame().bottom + 8.0f;
	boxRect.bottom = boxRect.top + 105;
	box = new BBox(boxRect, "", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW | B_FRAME_EVENTS
			| B_PULSE_NEEDED | B_NAVIGABLE_JUMP);
	box->SetLabel("Disk Mounting During Boot");
	AddChild(box);

	checkBoxRect = box->Bounds();
	checkBoxRect.InsetBy(8.0f, 8.0f);
	checkBoxRect.top += fontHeight.ascent + fontHeight.descent;

	checkBoxRect.bottom = checkBoxRect.top + 20;
	fInitialDontMountCheck = new BRadioButton(checkBoxRect, "initialNone",
		"Only The Boot Disk", new BMessage(kBootMountSettingsChanged));
	fInitialDontMountCheck->ResizeToPreferred();
	box->AddChild(fInitialDontMountCheck);

	checkBoxRect.OffsetBy(0, fInitialDontMountCheck->Bounds().Height());
	fInitialMountRestoreCheck = new BRadioButton(checkBoxRect, "initialRestore",
		"Previously Mounted Disks", new BMessage(kBootMountSettingsChanged));
	fInitialMountRestoreCheck->ResizeToPreferred();
	box->AddChild(fInitialMountRestoreCheck);

	checkBoxRect.OffsetBy(0, fInitialDontMountCheck->Bounds().Height());
	fInitialMountAllBFSCheck = new BRadioButton(checkBoxRect, "initialBFS",
		"All BeOS Disks", new BMessage(kBootMountSettingsChanged));
	fInitialMountAllBFSCheck->ResizeToPreferred();
	box->AddChild(fInitialMountAllBFSCheck);

	checkBoxRect.OffsetBy(0, fInitialDontMountCheck->Bounds().Height());
	fInitialMountAllCheck = new BRadioButton(checkBoxRect, "initialAll",
		"All Disks", new BMessage(kBootMountSettingsChanged));
	fInitialMountAllCheck->ResizeToPreferred();
	box->AddChild(fInitialMountAllCheck);

	box->ResizeTo(box->Bounds().Width(), fInitialMountAllCheck->Frame().bottom + 8.0f);

	// Buttons

	fDone = new BButton(Bounds(), "done", "Done", new BMessage(B_QUIT_REQUESTED),
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fDone->ResizeToPreferred();
	fDone->MoveTo(Bounds().Width() - fDone->Bounds().Width() - 8.0f,
		Bounds().bottom - fDone->Bounds().Height() - 8.0f);
	AddChild(fDone);

	fMountAllNow = new BButton(fDone->Frame(), "mountAll", "Mount all disks now",
		new BMessage(kMountAllNow), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fMountAllNow->ResizeToPreferred();
	fMountAllNow->MoveBy(-fMountAllNow->Bounds().Width() - 10.0f, 0.0f);
	AddChild(fMountAllNow);

	fDone->MakeDefault(true);

	bool result;
	if (settings->FindBool("autoMountAll", &result) == B_OK && result)
		fAutoMountAllCheck->SetValue(1);
	else if (settings->FindBool("autoMountAllBFS", &result) == B_OK && result)
		fAutoMountAllBFSCheck->SetValue(1);
	else
		fScanningDisabledCheck->SetValue(1);

	if (settings->FindBool("suspended", &result) == B_OK && result)
		fScanningDisabledCheck->SetValue(1);

	if (settings->FindBool("initialMountAll", &result) == B_OK && result)
		fInitialMountAllCheck->SetValue(1);
	else if (settings->FindBool("initialMountRestore", &result) == B_OK && result)
		fInitialMountRestoreCheck->SetValue(1);
	else if (settings->FindBool("initialMountAllBFS", &result) == B_OK && result)
		fInitialMountAllBFSCheck->SetValue(1);
	else
		fInitialDontMountCheck->SetValue(1);
}


AutomountSettingsPanel::~AutomountSettingsPanel()
{
}


void
AutomountSettingsPanel::SendSettings(bool rescan)
{
	BMessage message(kSetAutomounterParams);

	message.AddBool("autoMountAll", (bool)fAutoMountAllCheck->Value());
	message.AddBool("autoMountAllBFS", (bool)fAutoMountAllBFSCheck->Value());
	if (fAutoMountAllBFSCheck->Value())
		message.AddBool("autoMountAllHFS", false);

	message.AddBool("suspended", (bool)fScanningDisabledCheck->Value());
	message.AddBool("rescanNow", rescan);

	message.AddBool("initialMountAll", (bool)fInitialMountAllCheck->Value());
	message.AddBool("initialMountAllBFS", (bool)fInitialMountAllBFSCheck->Value());
	message.AddBool("initialMountRestore", (bool)fInitialMountRestoreCheck->Value());
	if (fInitialDontMountCheck->Value()) 
		message.AddBool("initialMountAllHFS", false);

	fTarget->PostMessage(&message, NULL);
}


void
AutomountSettingsPanel::GetPreferredSize(float* _width, float* _height)
{
	BBox* box = dynamic_cast<BBox*>(fInitialMountRestoreCheck->Parent());

	if (_width) {
		float a = fInitialMountRestoreCheck->Bounds().Width() + 32.0f;
		float b = box != NULL ? box->StringWidth(box->Label()) + 24.0f : 0.0f;
		float c = fMountAllNow->Bounds().Width() + fDone->Bounds().Width() + 26.0f;

		a = max_c(a, b);
		*_width = max_c(a, c);
	}

	if (_height) {
		*_height = (box != NULL ? box->Frame().bottom : 0.0f) + 16.0f
			+ fMountAllNow->Bounds().Height();
	}
}


void
AutomountSettingsPanel::AttachedToWindow()
{
	fInitialMountAllCheck->SetTarget(this);	
	fInitialMountAllBFSCheck->SetTarget(this);	
	fInitialMountRestoreCheck->SetTarget(this);	
	fInitialDontMountCheck->SetTarget(this);	

	fAutoMountAllCheck->SetTarget(this);	
	fAutoMountAllBFSCheck->SetTarget(this);	
	fScanningDisabledCheck->SetTarget(this);	

	fDone->SetTarget(this);	
	fMountAllNow->SetTarget(fTarget);
}


void
AutomountSettingsPanel::MessageReceived(BMessage *message)
{
	switch (message->what) {
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


//	#pragma mark -


AutomountSettingsDialog::AutomountSettingsDialog(BMessage *settings,  
		AutoMounter *target)
	: BWindow(BRect(100, 100, 320, 370), "Disk Mount Settings", 
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	BView* view = new AutomountSettingsPanel(Bounds(), settings, target);
	AddChild(view);

	float width, height;
	view->GetPreferredSize(&width, &height);
	ResizeTo(width, height);

	ASSERT(!sOneCopyOnly);
	sOneCopyOnly = this;
}


AutomountSettingsDialog::~AutomountSettingsDialog()
{
	ASSERT(sOneCopyOnly);
	sOneCopyOnly = NULL;
}


void
AutomountSettingsDialog::RunAutomountSettings(AutoMounter *target)
{
	// either activate an existing mount settings dialog or create a new one
	if (sOneCopyOnly) {
		sOneCopyOnly->Activate();
		return;
	}

	BMessage message;
	target->GetSettings(&message);
	(new AutomountSettingsDialog(&message, target))->Show();
}

