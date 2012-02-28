/*
 * Copyright 2004-2012, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		Humdinger <humdingerb@gmail.com>
 */

#include <Box.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Path.h>
#include <Screen.h>
#include <StringView.h>

#include "ExpanderPreferences.h"

const uint32 MSG_OK			= 'mgOK';
const uint32 MSG_CANCEL		= 'mCan';
const uint32 MSG_LEAVEDEST	= 'mLed';
const uint32 MSG_SAMEDIR	= 'mSad';
const uint32 MSG_DESTUSE	= 'mDeu';
const uint32 MSG_DESTTEXT	= 'mDet';
const uint32 MSG_DESTSELECT = 'mDes';

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "ExpanderPreferences"


ExpanderPreferences::ExpanderPreferences(BMessage* settings)
	: BWindow(BRect(0, 0, 325, 305), B_TRANSLATE("Expander settings"),
		B_FLOATING_WINDOW_LOOK,	B_FLOATING_APP_WINDOW_FEEL, B_NOT_CLOSABLE
			| B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fSettings(settings),
	fUsePanel(NULL)
{
	const float kSpacing = be_control_look->DefaultItemSpacing();

	BBox* settingsBox = new BBox(B_PLAIN_BORDER, NULL);
	BGroupLayout* settingsLayout = new BGroupLayout(B_VERTICAL, kSpacing / 2);
	settingsBox->SetLayout(settingsLayout);
	BBox* buttonBox = new BBox(B_PLAIN_BORDER, NULL);
	BGroupLayout* buttonLayout = new BGroupLayout(B_HORIZONTAL, kSpacing / 2);
	buttonBox->SetLayout(buttonLayout);

	BStringView* expansionLabel = new BStringView("stringViewExpansion",
		B_TRANSLATE("Expansion"));
	expansionLabel->SetFont(be_bold_font);
	BStringView* destinationLabel = new BStringView("stringViewDestination",
		B_TRANSLATE("Destination folder"));
	destinationLabel->SetFont(be_bold_font);
	BStringView* otherLabel = new BStringView("stringViewOther",
		B_TRANSLATE("Other"));
	otherLabel->SetFont(be_bold_font);

	fAutoExpand = new BCheckBox("autoExpand",
		B_TRANSLATE("Automatically expand files"), NULL);
	fCloseWindow = new BCheckBox("closeWindowWhenDone",
		B_TRANSLATE("Close window when done expanding"), NULL);

	fLeaveDest = new BRadioButton("leaveDest",
		B_TRANSLATE("Leave destination folder path empty"),
		new BMessage(MSG_LEAVEDEST));
	fSameDest = new BRadioButton("sameDir",
		B_TRANSLATE("Same directory as source (archive) file"),
		new BMessage(MSG_SAMEDIR));
	fDestUse = new BRadioButton("destUse",
		B_TRANSLATE("Use:"), new BMessage(MSG_DESTUSE));
	fDestText = new BTextControl("destText", "", "",
		new BMessage(MSG_DESTTEXT));
	fDestText->SetDivider(0);
	fDestText->TextView()->MakeEditable(false);
	fDestText->SetEnabled(false);
	fSelect = new BButton("selectButton", B_TRANSLATE("Select"),
		new BMessage(MSG_DESTSELECT));
	fSelect->SetEnabled(false);

	fOpenDest = new BCheckBox("openDestination",
		B_TRANSLATE("Open destination folder after extraction"), NULL);
	fAutoShow = new BCheckBox("autoShow",
		B_TRANSLATE("Automatically show contents listing"), NULL);

	BButton* okbutton = new BButton("OKButton", B_TRANSLATE("OK"),
		new BMessage(MSG_OK));
	okbutton->MakeDefault(true);
	BButton* cancel = new BButton("CancelButton", B_TRANSLATE("Cancel"),
		new BMessage(MSG_CANCEL));

	// Build the layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.AddGroup(settingsLayout)
			.AddGroup(B_HORIZONTAL)
				.Add(expansionLabel)
				.AddGlue()
			.End()
			.AddGroup(B_VERTICAL, 0)
				.Add(fAutoExpand)
				.Add(fCloseWindow)
				.SetInsets(kSpacing, 0, 0, 0)
			.End()
			.AddGroup(B_HORIZONTAL, 0)
				.Add(destinationLabel)
				.AddGlue()
				.SetInsets(0, kSpacing, 0, 0)
			.End()
			.AddGroup(B_VERTICAL, 0)
				.Add(fLeaveDest)
				.Add(fSameDest)
				.Add(fDestUse)
				.AddGroup(B_HORIZONTAL, 0)
					.Add(fDestText, 0.8)
					.AddStrut(be_control_look->DefaultLabelSpacing())
					.Add(fSelect, 0.2)
					.SetInsets(kSpacing * 2, 0, kSpacing / 2, 0)
				.End()
				.SetInsets(kSpacing, 0, 0, 0)
			.End()
			.AddGroup(B_HORIZONTAL, 0)
				.Add(otherLabel)
				.AddGlue()
				.SetInsets(0, kSpacing / 2, 0, 0)
			.End()
			.AddGroup(B_VERTICAL, 0)
				.Add(fOpenDest)
				.Add(fAutoShow)
				.SetInsets(kSpacing, 0, 0, 0)
			.End()
			.SetInsets(kSpacing, kSpacing, kSpacing, kSpacing)
		.End()
		.AddGroup(buttonLayout)
			.AddGroup(B_HORIZONTAL, kSpacing)
				.AddGlue()
				.Add(cancel)
				.Add(okbutton)
			.End()
			.SetInsets(kSpacing, kSpacing, kSpacing, kSpacing)
		.End();

	CenterOnScreen();

	_ReadSettings();
}


ExpanderPreferences::~ExpanderPreferences()
{
	if (fUsePanel && fUsePanel->RefFilter())
		delete fUsePanel->RefFilter();

	delete fUsePanel;
}


void
ExpanderPreferences::_ReadSettings()
{
	bool automatically_expand_files;
	bool close_when_done;
	int8 destination_folder;
	entry_ref ref;
	bool open_destination_folder;
	bool show_contents_listing;
	if ((fSettings->FindBool("automatically_expand_files",
			&automatically_expand_files) == B_OK)
				&& automatically_expand_files)
		fAutoExpand->SetValue(B_CONTROL_ON);

	if ((fSettings->FindBool("close_when_done", &close_when_done) == B_OK)
			&& close_when_done)
		fCloseWindow->SetValue(B_CONTROL_ON);

	if (fSettings->FindInt8("destination_folder", &destination_folder) == B_OK) {
		switch (destination_folder) {
			case 0x63:
				fSameDest->SetValue(B_CONTROL_ON);
				break;
			case 0x65:
				fDestUse->SetValue(B_CONTROL_ON);
				fDestText->SetEnabled(true);
				fSelect->SetEnabled(true);
				break;
			case 0x66:
				fLeaveDest->SetValue(B_CONTROL_ON);
				break;
		}
	}

	if (fSettings->FindRef("destination_folder_use", &fRef) == B_OK) {
		BEntry entry(&fRef);
		if (entry.Exists()) {
			BPath path(&entry);
			fDestText->SetText(path.Path());
		}
	}

	if ((fSettings->FindBool("open_destination_folder",
			&open_destination_folder) == B_OK)
				&& open_destination_folder)
		fOpenDest->SetValue(B_CONTROL_ON);

	if ((fSettings->FindBool("show_contents_listing",
			&show_contents_listing) == B_OK)
				&& show_contents_listing)
		fAutoShow->SetValue(B_CONTROL_ON);
}


void
ExpanderPreferences::_WriteSettings()
{
	fSettings->ReplaceBool("automatically_expand_files",
		fAutoExpand->Value() == B_CONTROL_ON);
	fSettings->ReplaceBool("close_when_done",
		fCloseWindow->Value() == B_CONTROL_ON);
	fSettings->ReplaceInt8("destination_folder",
		(fSameDest->Value() == B_CONTROL_ON) ? 0x63
		: ((fLeaveDest->Value() == B_CONTROL_ON) ? 0x66 : 0x65));
	fSettings->ReplaceRef("destination_folder_use", &fRef);
	fSettings->ReplaceBool("open_destination_folder",
		fOpenDest->Value() == B_CONTROL_ON);
	fSettings->ReplaceBool("show_contents_listing",
		fAutoShow->Value() == B_CONTROL_ON);
}


void
ExpanderPreferences::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case MSG_DESTSELECT:
		{
			if (!fUsePanel) {
				BMessenger messenger(this);
				fUsePanel = new DirectoryFilePanel(B_OPEN_PANEL, &messenger, NULL,
					B_DIRECTORY_NODE, false, NULL, new DirectoryRefFilter(), true);
			}
			fUsePanel->Show();
			break;
		}
		case MSG_DIRECTORY:
		{
			entry_ref ref;
			fUsePanel->GetPanelDirectory(&ref);
			fRef = ref;
			BEntry entry(&ref);
			BPath path(&entry);
			fDestText->SetText(path.Path());
			fUsePanel->Hide();
			break;
		}
		case B_REFS_RECEIVED:
		{
			if (msg->FindRef("refs", 0, &fRef) == B_OK) {
				BEntry entry(&fRef, true);
				BPath path(&entry);
				fDestText->SetText(path.Path());
			}
			break;
		}
		case MSG_LEAVEDEST:
		case MSG_SAMEDIR:
		{
			fDestText->SetEnabled(false);
			fSelect->SetEnabled(false);
			break;
		}
		case MSG_DESTUSE:
		{
			fDestText->SetEnabled(true);
			fSelect->SetEnabled(true);
			fDestText->TextView()->MakeEditable(false);
			break;
		}
		case MSG_CANCEL:
			_ReadSettings();
			Hide();
			break;

		case MSG_OK:
			_WriteSettings();
			Hide();
			break;

		default:
			break;
	}
}
