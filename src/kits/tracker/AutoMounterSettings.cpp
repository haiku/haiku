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

#include "AutoMounterSettings.h"

#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <Debug.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Message.h>
#include <RadioButton.h>
#include <SeparatorView.h>
#include <SpaceLayoutItem.h>
#include <Window.h>

#include <MountServer.h>


const uint32 kAutomountSettingsChanged = 'achg';
const uint32 kBootMountSettingsChanged = 'bchg';
const uint32 kEjectWhenUnmountingChanged = 'ejct';

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AutoMounterSettings"

class AutomountSettingsPanel : public BBox {
public:
								AutomountSettingsPanel(BMessage* settings,
									const BMessenger& target);
	virtual						~AutomountSettingsPanel();

protected:
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				AttachedToWindow();

private:
			void				_SendSettings(bool rescan);

			BRadioButton*		fInitialDontMountCheck;
			BRadioButton*		fInitialMountAllBFSCheck;
			BRadioButton*		fInitialMountAllCheck;
			BRadioButton*		fInitialMountRestoreCheck;

			BRadioButton*		fScanningDisabledCheck;
			BRadioButton*		fAutoMountAllBFSCheck;
			BRadioButton*		fAutoMountAllCheck;

			BCheckBox*			fEjectWhenUnmountingCheckBox;

			BButton*			fDone;
			BButton*			fMountAllNow;

			BMessenger			fTarget;

			typedef BBox _inherited;
};


AutomountSettingsDialog* AutomountSettingsDialog::sOneCopyOnly = NULL;


AutomountSettingsPanel::AutomountSettingsPanel(BMessage* settings,
		const BMessenger& target)
	:
	BBox("", B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_NO_BORDER),
	fTarget(target)
{
	const float spacing = be_control_look->DefaultItemSpacing();

	// "Automatic Disk Mounting" group

	BBox* autoMountBox = new BBox("autoMountBox", B_WILL_DRAW | B_FRAME_EVENTS
		| B_PULSE_NEEDED | B_NAVIGABLE_JUMP);
	autoMountBox->SetLabel(B_TRANSLATE("Automatic disk mounting"));
	BGroupLayout* autoMountLayout = new BGroupLayout(B_VERTICAL, 0);
	autoMountBox->SetLayout(autoMountLayout);
	autoMountLayout->SetInsets(spacing,
		autoMountBox->InnerFrame().top + spacing / 2, spacing, spacing);

	fScanningDisabledCheck = new BRadioButton("scanningOff",
		B_TRANSLATE("Don't automount"),
		new BMessage(kAutomountSettingsChanged));

	fAutoMountAllBFSCheck = new BRadioButton("autoBFS",
		B_TRANSLATE("All BeOS disks"), new BMessage(kAutomountSettingsChanged));

	fAutoMountAllCheck = new BRadioButton("autoAll",
		B_TRANSLATE("All disks"), new BMessage(kAutomountSettingsChanged));

	// "Disk Mounting During Boot" group

	BBox* bootMountBox = new BBox("", B_WILL_DRAW | B_FRAME_EVENTS
		| B_PULSE_NEEDED | B_NAVIGABLE_JUMP);
	bootMountBox->SetLabel(B_TRANSLATE("Disk mounting during boot"));
	BGroupLayout* bootMountLayout = new BGroupLayout(B_VERTICAL, 0);
	bootMountBox->SetLayout(bootMountLayout);
	bootMountLayout->SetInsets(spacing,
		bootMountBox->InnerFrame().top + spacing / 2, spacing, spacing);

	fInitialDontMountCheck = new BRadioButton("initialNone",
		B_TRANSLATE("Only the boot disk"),
		new BMessage(kBootMountSettingsChanged));

	fInitialMountRestoreCheck = new BRadioButton("initialRestore",
		B_TRANSLATE("Previously mounted disks"),
		new BMessage(kBootMountSettingsChanged));

	fInitialMountAllBFSCheck = new BRadioButton("initialBFS",
		B_TRANSLATE("All BeOS disks"),
		new BMessage(kBootMountSettingsChanged));

	fInitialMountAllCheck = new BRadioButton("initialAll",
		B_TRANSLATE("All disks"), new BMessage(kBootMountSettingsChanged));

	fEjectWhenUnmountingCheckBox = new BCheckBox("ejectWhenUnmounting",
		B_TRANSLATE("Eject when unmounting"),
		new BMessage(kEjectWhenUnmountingChanged));

	// Buttons

	fDone = new BButton(B_TRANSLATE("Done"), new BMessage(B_QUIT_REQUESTED));

	fMountAllNow = new BButton("mountAll", B_TRANSLATE("Mount all disks now"),
		new BMessage(kMountAllNow));

	fDone->MakeDefault(true);

	// Layout the controls
	BGroupView* contentView = new BGroupView(B_VERTICAL, 0);
	AddChild(contentView);
	BLayoutBuilder::Group<>(contentView)
		.AddGroup(B_VERTICAL, spacing)
			.SetInsets(spacing, spacing, spacing, spacing)
			.AddGroup(autoMountLayout)
				.Add(fScanningDisabledCheck)
				.Add(fAutoMountAllBFSCheck)
				.Add(fAutoMountAllCheck)
				.End()
			.AddGroup(bootMountLayout)
				.Add(fInitialDontMountCheck)
				.Add(fInitialMountRestoreCheck)
				.Add(fInitialMountAllBFSCheck)
				.Add(fInitialMountAllCheck)
				.End()
			.AddGroup(B_HORIZONTAL)
				.AddStrut(spacing - 1)
				.Add(fEjectWhenUnmountingCheckBox)
				.End()
			.End()
		.Add(new BSeparatorView(B_HORIZONTAL/*, B_FANCY_BORDER*/))
		.AddGroup(B_HORIZONTAL, spacing)
			.SetInsets(0, spacing, spacing, spacing)
			.AddGlue()
			.Add(fMountAllNow)
			.Add(fDone);

	// Apply the settings

	bool result;
	if (settings->FindBool("autoMountAll", &result) == B_OK && result)
		fAutoMountAllCheck->SetValue(B_CONTROL_ON);
	else if (settings->FindBool("autoMountAllBFS", &result) == B_OK && result)
		fAutoMountAllBFSCheck->SetValue(B_CONTROL_ON);
	else
		fScanningDisabledCheck->SetValue(B_CONTROL_ON);

	if (settings->FindBool("suspended", &result) == B_OK && result)
		fScanningDisabledCheck->SetValue(B_CONTROL_ON);

	if (settings->FindBool("initialMountAll", &result) == B_OK && result)
		fInitialMountAllCheck->SetValue(B_CONTROL_ON);
	else if (settings->FindBool("initialMountRestore", &result) == B_OK
		&& result) {
		fInitialMountRestoreCheck->SetValue(B_CONTROL_ON);
	} else if (settings->FindBool("initialMountAllBFS", &result) == B_OK
		&& result) {
		fInitialMountAllBFSCheck->SetValue(B_CONTROL_ON);
	} else
		fInitialDontMountCheck->SetValue(B_CONTROL_ON);

	if (settings->FindBool("ejectWhenUnmounting", &result) == B_OK && result)
		fEjectWhenUnmountingCheckBox->SetValue(B_CONTROL_ON);
}


AutomountSettingsPanel::~AutomountSettingsPanel()
{
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
	fEjectWhenUnmountingCheckBox->SetTarget(this);

	fDone->SetTarget(this);
	fMountAllNow->SetTarget(fTarget);
}


void
AutomountSettingsPanel::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_QUIT_REQUESTED:
			Window()->Quit();
			break;

		case kAutomountSettingsChanged:
			_SendSettings(true);
			break;

		case kBootMountSettingsChanged:
		case kEjectWhenUnmountingChanged:
			_SendSettings(false);
			break;

		default:
			_inherited::MessageReceived(message);
			break;
	}
}


void
AutomountSettingsPanel::_SendSettings(bool rescan)
{
	BMessage message(kSetAutomounterParams);

	message.AddBool("autoMountAll", (bool)fAutoMountAllCheck->Value());
	message.AddBool("autoMountAllBFS", (bool)fAutoMountAllBFSCheck->Value());
	if (fAutoMountAllBFSCheck->Value())
		message.AddBool("autoMountAllHFS", false);

	message.AddBool("suspended", (bool)fScanningDisabledCheck->Value());
	message.AddBool("rescanNow", rescan);

	message.AddBool("initialMountAll", (bool)fInitialMountAllCheck->Value());
	message.AddBool("initialMountAllBFS",
		(bool)fInitialMountAllBFSCheck->Value());
	message.AddBool("initialMountRestore",
		(bool)fInitialMountRestoreCheck->Value());
	if (fInitialDontMountCheck->Value())
		message.AddBool("initialMountAllHFS", false);

	message.AddBool("ejectWhenUnmounting",
		(bool)fEjectWhenUnmountingCheckBox->Value());

	fTarget.SendMessage(&message);
}


//	#pragma mark -


AutomountSettingsDialog::AutomountSettingsDialog(BMessage* settings,
		const BMessenger& target)
	:
	BWindow(BRect(100, 100, 320, 370), B_TRANSLATE("Disk mount settings"),
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE
		| B_AUTO_UPDATE_SIZE_LIMITS)
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));
	BView* view = new AutomountSettingsPanel(settings, target);
	AddChild(view);

	ASSERT(!sOneCopyOnly);
	sOneCopyOnly = this;
}


AutomountSettingsDialog::~AutomountSettingsDialog()
{
	ASSERT(sOneCopyOnly);
	sOneCopyOnly = NULL;
}


void
AutomountSettingsDialog::RunAutomountSettings(const BMessenger& target)
{
	// either activate an existing mount settings dialog or create a new one
	if (sOneCopyOnly) {
		sOneCopyOnly->Activate();
		return;
	}

	BMessage message(kGetAutomounterParams);
	BMessage reply;
	status_t ret = target.SendMessage(&message, &reply, 2500000);
	if (ret != B_OK) {
		(new BAlert(B_TRANSLATE("Mount server error"),
			B_TRANSLATE("The mount server could not be contacted."),
			B_TRANSLATE("OK"),
			NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
		return;
	}

	(new AutomountSettingsDialog(&reply, target))->Show();
}

