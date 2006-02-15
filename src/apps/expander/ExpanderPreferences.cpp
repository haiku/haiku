/*
 * Copyright 2004-2006, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
 
#include "ExpanderPreferences.h"
#include <Box.h>
#include <Path.h>
#include <Screen.h>
#include <TextView.h>

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

	rect.OffsetBy(11,9);
	rect.bottom -= 64;
	rect.right -= 22;
	BBox *box = new BBox(rect, "background", B_FOLLOW_ALL, 
		B_WILL_DRAW | B_FRAME_EVENTS, B_FANCY_BORDER);
	box->SetLabel("Expander Preferences");
	background->AddChild(box);
	
	 
	BRect frameRect = box->Bounds();
	frameRect.OffsetBy(15,23);
	frameRect.right = frameRect.left + 100;
	frameRect.bottom = frameRect.top + 20;
	BRect textRect(frameRect);
	textRect.OffsetTo(B_ORIGIN);
	textRect.InsetBy(1,1);
	BTextView *textView = new BTextView(frameRect, "expansion", textRect,
		be_plain_font, NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	textView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	box->AddChild(textView);
	textView->SetText("Expansion:");
	
	frameRect.OffsetBy(0, 58);
	textRect.OffsetBy(0, 58);
	textView = new BTextView(frameRect, "destinationFolder", textRect,
		be_plain_font, NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	textView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	box->AddChild(textView);
	textView->SetText("Destination Folder:");
	
	frameRect.OffsetBy(0, 84);
	textRect.OffsetBy(0, 84);
	textView = new BTextView(frameRect, "other", textRect,
		be_plain_font, NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	textView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	box->AddChild(textView);
	textView->SetText("Other:");
	
	rect = box->Bounds();
	rect.OffsetBy(25, 42);
	rect.bottom = rect.top + 20;
	rect.right = rect.right - 30;
	fAutoExpand = new BCheckBox(rect, "autoExpand", "Automatically expand files", NULL);
	box->AddChild(fAutoExpand);
	
	rect.OffsetBy(0, 17);
	fCloseWindow = new BCheckBox(rect, "closeWindowWhenDone", "Close window when done expanding", NULL);
	box->AddChild(fCloseWindow);
	
	rect.OffsetBy(0, 50);
	fLeaveDest = new BRadioButton(rect, "leaveDest", "Leave destination folder path empty", 
		new BMessage(MSG_LEAVEDEST));
	box->AddChild(fLeaveDest);
	
	rect.OffsetBy(0, 17);
	fSameDest = new BRadioButton(rect, "sameDir", "Same directory as source (archive) file", 
		new BMessage(MSG_SAMEDIR));
	box->AddChild(fSameDest);
	
	rect.OffsetBy(0, 17);
	BRect useRect = rect;
	useRect.right = useRect.left + 50;
	fDestUse = new BRadioButton(useRect, "destUse", "Use:", 
		new BMessage(MSG_DESTUSE));
	box->AddChild(fDestUse);
	
	textRect = rect;
	textRect.OffsetBy(45, 0);
	textRect.right = textRect.left + 158;
	fDestText = new BTextControl(textRect, "destText", "", "", new BMessage(MSG_DESTTEXT));
	fDestText->SetDivider(0);
	fDestText->TextView()->MakeEditable(false);
	box->AddChild(fDestText);
	fDestText->SetEnabled(false);
	
	textRect.OffsetBy(168, -4);
	textRect.right = textRect.left + 50;
	fSelect = new BButton(textRect, "selectButton", "Select", new BMessage(MSG_DESTSELECT));
	box->AddChild(fSelect);
	fSelect->SetEnabled(false);
		
	rect.OffsetBy(0, 50);
	fOpenDest = new BCheckBox(rect, "openDestination", "Open destination folder after extraction", NULL);
	box->AddChild(fOpenDest);
	
	rect.OffsetBy(0, 17);
	fAutoShow = new BCheckBox(rect, "autoShow", "Automatically show contents listing", NULL);
	box->AddChild(fAutoShow);
	
	rect = BRect(Bounds().right-89, Bounds().bottom-40,
		Bounds().right-14, Bounds().bottom-16);
	BButton *button = new BButton(rect, "OKButton", "OK", 
		new BMessage(MSG_OK));
	background->AddChild(button);
	button->MakeDefault(true);
	
	rect.OffsetBy(-89, 0);
	button = new BButton(rect, "CancelButton", "Cancel", 
		new BMessage(MSG_CANCEL));
	background->AddChild(button);
	
	BScreen screen(this);
	MoveBy((screen.Frame().Width()-Bounds().Width())/2, 
		(screen.Frame().Height()-Bounds().Height())/2);
	
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
				: ( (fLeaveDest->Value() == B_CONTROL_ON) ? 0x66 : 0x65) );
			fSettings->ReplaceRef("destination_folder_use", &fRef);
			fSettings->ReplaceBool("open_destination_folder", fOpenDest->Value() == B_CONTROL_ON);
			fSettings->ReplaceBool("show_contents_listing", fAutoShow->Value() == B_CONTROL_ON);
			Hide();
			break;
		default:
			break;
	}
}
