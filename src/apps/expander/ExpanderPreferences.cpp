/*
 * Copyright 2004-2006, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ExpanderPreferences.h"
#include <Box.h>
#include <Path.h>
#include <Screen.h>
#include <StringView.h>

const uint32 MSG_OK			= 'mgOK';
const uint32 MSG_CANCEL		= 'mCan';
const uint32 MSG_LEAVEDEST = 'mLed';
const uint32 MSG_SAMEDIR = 'mSad';
const uint32 MSG_DESTUSE = 'mDeu';
const uint32 MSG_DESTTEXT = 'mDet';
const uint32 MSG_DESTSELECT = 'mDes';

ExpanderPreferences::ExpanderPreferences(BMessage *settings)
	: BWindow(BRect(0, 0, 325, 305), "Expand-O-Matic", B_MODAL_WINDOW,
		B_NOT_CLOSABLE | B_NOT_RESIZABLE),
	fSettings(settings),
	fUsePanel(NULL)
{

	BRect rect = Bounds();
	BBox *background = new BBox(rect, "background", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER);
	AddChild(background);

	rect.OffsetBy(11, 9);
	rect.bottom -= 64;
	rect.right -= 22;
	BBox *box = new BBox(rect, "background", B_FOLLOW_NONE,
		B_WILL_DRAW | B_FRAME_EVENTS, B_FANCY_BORDER);
	box->SetLabel("Expander Preferences");
	background->AddChild(box);

	float maxWidth = box->Bounds().right;

	BRect frameRect = box->Bounds();
	frameRect.OffsetBy(15, 23);
	frameRect.right = frameRect.left + 200;
	frameRect.bottom = frameRect.top + 20;
	BRect textRect(frameRect);
	textRect.OffsetTo(B_ORIGIN);
	textRect.InsetBy(1, 1);
	BStringView *stringView = new BStringView(frameRect, "expansion", "Expansion:",
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	stringView->ResizeToPreferred();
	if (stringView->Frame().right > maxWidth)
		maxWidth = stringView->Frame().right;
	box->AddChild(stringView);

	frameRect.top = stringView->Frame().bottom + 5;
	frameRect.left += 10;

	fAutoExpand = new BCheckBox(frameRect, "autoExpand", "Automatically expand files", NULL);
	fAutoExpand->ResizeToPreferred();
	if (fAutoExpand->Frame().right > maxWidth)
		maxWidth = fAutoExpand->Frame().right;
	box->AddChild(fAutoExpand);

	frameRect = fAutoExpand->Frame();
	frameRect.top = fAutoExpand->Frame().bottom + 1;
	fCloseWindow = new BCheckBox(frameRect, "closeWindowWhenDone", "Close window when done expanding", NULL);
	fCloseWindow->ResizeToPreferred();
	if (fCloseWindow->Frame().right > maxWidth)
		maxWidth = fCloseWindow->Frame().right;
	box->AddChild(fCloseWindow);

	frameRect = stringView->Frame();
	frameRect.top = fCloseWindow->Frame().bottom + 10;
	stringView = new BStringView(frameRect, "destinationFolder", "Destination Folder:",
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	stringView->ResizeToPreferred();
	if (stringView->Frame().right > maxWidth)
		maxWidth = stringView->Frame().right;
	box->AddChild(stringView);

	frameRect.top = stringView->Frame().bottom + 5;
	frameRect.left += 10;

	fLeaveDest = new BRadioButton(frameRect, "leaveDest", "Leave destination folder path empty",
		new BMessage(MSG_LEAVEDEST));
	fLeaveDest->ResizeToPreferred();
	if (fLeaveDest->Frame().right > maxWidth)
		maxWidth = fLeaveDest->Frame().right;
	box->AddChild(fLeaveDest);

	frameRect = fLeaveDest->Frame();
	frameRect.top = fLeaveDest->Frame().bottom + 1;
	fSameDest = new BRadioButton(frameRect, "sameDir", "Same directory as source (archive) file",
		new BMessage(MSG_SAMEDIR));
	fSameDest->ResizeToPreferred();
	if (fSameDest->Frame().right > maxWidth)
		maxWidth = fSameDest->Frame().right;
	box->AddChild(fSameDest);

	frameRect = fSameDest->Frame();
	frameRect.top = frameRect.bottom + 1;
	fDestUse = new BRadioButton(frameRect, "destUse", "Use:", new BMessage(MSG_DESTUSE));
	fDestUse->ResizeToPreferred();
	if (fDestUse->Frame().right > maxWidth)
		maxWidth = fDestUse->Frame().right;
	box->AddChild(fDestUse);

	frameRect = fDestUse->Frame();
	frameRect.left  = fDestUse->Frame().right + 1;
	frameRect.right  = frameRect.left + 58;
	frameRect.bottom  = frameRect.top + 38;

	fDestText = new BTextControl(frameRect, "destText", "", "", new BMessage(MSG_DESTTEXT));
	box->AddChild(fDestText);
	fDestText->ResizeToPreferred();
	fDestText->SetDivider(0);
	fDestText->TextView()->MakeEditable(false);
	fDestText->ResizeTo(158, fDestText->Frame().Height());

	fDestText->SetEnabled(false);

	frameRect = fDestText->Frame();
	frameRect.left = frameRect.right + 5;
	fSelect = new BButton(frameRect, "selectButton", "Select", new BMessage(MSG_DESTSELECT));
	fSelect->ResizeToPreferred();
	if (fSelect->Frame().right > maxWidth)
		maxWidth = fSelect->Frame().right;
	box->AddChild(fSelect);
	fSelect->SetEnabled(false);

	fDestText->MoveBy(0, (fSelect->Frame().Height() - fDestText->Frame().Height()) / 2.0);
	fDestText->ResizeTo(158, fDestText->Frame().Height());

	frameRect = stringView->Frame();
	frameRect.top = fDestUse->Frame().bottom + 10;

	stringView = new BStringView(frameRect, "other", "Other:", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	stringView->ResizeToPreferred();
	if (stringView->Frame().right > maxWidth)
		maxWidth = stringView->Frame().right;
	box->AddChild(stringView);

	frameRect.top = stringView->Frame().bottom + 5;
	frameRect.left += 10;

	fOpenDest = new BCheckBox(frameRect, "openDestination", "Open destination folder after extraction", NULL);
	fOpenDest->ResizeToPreferred();
	if (fOpenDest->Frame().right > maxWidth)
		maxWidth = fOpenDest->Frame().right;
	box->AddChild(fOpenDest);

	frameRect = fOpenDest->Frame();
	frameRect.top = frameRect.bottom + 1;
	fAutoShow = new BCheckBox(frameRect, "autoShow", "Automatically show contents listing", NULL);
	fAutoShow->ResizeToPreferred();
	if (fAutoShow->Frame().right > maxWidth)
		maxWidth = fAutoShow->Frame().right;
	box->AddChild(fAutoShow);

	box->ResizeTo(maxWidth + 15, fAutoShow->Frame().bottom + 10);

	rect = BRect(Bounds().right - 89, Bounds().bottom - 40, Bounds().right - 14, Bounds().bottom - 16);

	rect = Bounds();
	BButton *button = new BButton(rect, "OKButton", "OK", new BMessage(MSG_OK));
	button->MakeDefault(true);
	button->ResizeToPreferred();
	button->MoveTo(box->Frame().right - button->Frame().Width(), box->Frame().bottom + 10);
	background->AddChild(button);

	rect = button->Frame();
	BButton *cancel = new BButton(rect, "CancelButton", "Cancel", new BMessage(MSG_CANCEL));
	cancel->ResizeToPreferred();
	cancel->MoveBy(-cancel->Frame().Width() - 10, (button->Frame().Height() - cancel->Frame().Height()) / 2.0);
	background->AddChild(cancel);

	ResizeTo(box->Frame().right + 11 , button->Frame().bottom + 11);

	BScreen screen(this);
	MoveBy((screen.Frame().Width() - Bounds().Width()) / 2,
		(screen.Frame().Height() - Bounds().Height()) / 2);

	bool automatically_expand_files;
	bool close_when_done;
	int8 destination_folder;
	entry_ref ref;
	bool open_destination_folder;
	bool show_contents_listing;
	if ((settings->FindBool("automatically_expand_files", &automatically_expand_files) == B_OK)
		&& automatically_expand_files)
		fAutoExpand->SetValue(B_CONTROL_ON);

	if ((settings->FindBool("close_when_done", &close_when_done) == B_OK)
		&& close_when_done)
		fCloseWindow->SetValue(B_CONTROL_ON);

	if (settings->FindInt8("destination_folder", &destination_folder) == B_OK) {
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

	if (settings->FindRef("destination_folder_use", &fRef) == B_OK) {
		BEntry entry(&fRef);
		if (entry.Exists()) {
			BPath path(&entry);
			fDestText->SetText(path.Path());
		}
	}

	if ((settings->FindBool("open_destination_folder", &open_destination_folder) == B_OK)
		&& open_destination_folder)
		fOpenDest->SetValue(B_CONTROL_ON);

	if ((settings->FindBool("show_contents_listing", &show_contents_listing) == B_OK)
		&& show_contents_listing)
		fAutoShow->SetValue(B_CONTROL_ON);
}


ExpanderPreferences::~ExpanderPreferences()
{
}


void
ExpanderPreferences::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case MSG_DESTSELECT:
			if (!fUsePanel)
				fUsePanel = new DirectoryFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL,
					B_DIRECTORY_NODE, false, NULL, new DirectoryRefFilter(), true);
			fUsePanel->Show();
			break;
		case MSG_DIRECTORY:
		{
			entry_ref ref;
			fUsePanel->GetPanelDirectory(&ref);
			fRef = ref;
			BEntry entry(&ref);
			BPath path(&entry);
			fDestText->SetText(path.Path());
			fUsePanel->Hide();
		}
		break;
		case B_REFS_RECEIVED:
			if (msg->FindRef("refs", 0, &fRef) == B_OK) {
				BEntry entry(&fRef, true);
				BPath path(&entry);
				fDestText->SetText(path.Path());
			}
			break;
		case MSG_LEAVEDEST:
		case MSG_SAMEDIR:
			fDestText->SetEnabled(false);
			fSelect->SetEnabled(false);
			break;
		case MSG_DESTUSE:
			fDestText->SetEnabled(true);
			fSelect->SetEnabled(true);
			fDestText->TextView()->MakeEditable(false);
			break;
		case MSG_CANCEL:
			Hide();
			break;
		case MSG_OK:
			fSettings->ReplaceBool("automatically_expand_files", fAutoExpand->Value() == B_CONTROL_ON);
			fSettings->ReplaceBool("close_when_done", fCloseWindow->Value() == B_CONTROL_ON);
			fSettings->ReplaceInt8("destination_folder", (fSameDest->Value() == B_CONTROL_ON) ? 0x63
				: ((fLeaveDest->Value() == B_CONTROL_ON) ? 0x66 : 0x65));
			fSettings->ReplaceRef("destination_folder_use", &fRef);
			fSettings->ReplaceBool("open_destination_folder", fOpenDest->Value() == B_CONTROL_ON);
			fSettings->ReplaceBool("show_contents_listing", fAutoShow->Value() == B_CONTROL_ON);
			Hide();
			break;
		default:
			break;
	}
}
