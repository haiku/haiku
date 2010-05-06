/*
 * Copyright 2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */


#include "FileSelectionPage.h"


#include <Button.h>
#include <Catalog.h>
#include <Path.h>
#include <RadioButton.h>
#include <TextControl.h>
#include <TextView.h>

#include <string.h>
#include <String.h>


#undef TR_CONTEXT
#define TR_CONTEXT "FileSelectionPage"


const uint32 kMsgOpenFilePanel = 'open';


FileSelectionPage::FileSelectionPage(BMessage* settings, BRect frame, const char* name,
		const char* description, file_panel_mode mode)
	: WizardPageView(settings, frame, name, B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE)
	, fMode(mode)
	, fFilePanel(NULL)
{
	_BuildUI(description);
}


FileSelectionPage::~FileSelectionPage()
{
}


void
FileSelectionPage::FrameResized(float width, float height)
{
	WizardPageView::FrameResized(width, height);
	_Layout();
}


void
FileSelectionPage::AttachedToWindow()
{
	fSelect->SetTarget(this);
}


void
FileSelectionPage::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgOpenFilePanel:
			_OpenFilePanel();
			break;
		case B_REFS_RECEIVED:
		case B_SAVE_REQUESTED:
			_SetFileFromFilePanelMessage(message);
			break;
		case B_CANCEL:
			_FilePanelCanceled();
			break;
		default:
			WizardPageView::MessageReceived(message);
	}
}


void
FileSelectionPage::PageCompleted()
{
	fSettings->ReplaceString("file", fFile->Text());
}


const float kTextDistance = 10;
const float kFileButtonDistance = 10;

void
FileSelectionPage::_BuildUI(const char* description)
{
	BRect rect(Bounds());

	fDescription = CreateDescription(rect, "description", description);

	MakeHeading(fDescription);
	AddChild(fDescription);

	BString file;
	fSettings->FindString("file", &file);

	fSelect = new BButton(rect, "select",
		B_TRANSLATE_COMMENT("Select", "Button"),
		new BMessage(kMsgOpenFilePanel),
		B_FOLLOW_RIGHT);
	fSelect->ResizeToPreferred();

	float selectLeft = rect.right - fSelect->Bounds().Width();
	rect.right = selectLeft - kFileButtonDistance;
	fFile = new BTextControl(rect, "file",
		B_TRANSLATE_COMMENT("File:", "Text control label"),
		file.String(), new BMessage());
	fFile->SetDivider(be_plain_font->StringWidth(fFile->Label()) + 5);
	AddChild(fFile);

	fSelect->MoveTo(selectLeft, 0);
	AddChild(fSelect);

	_Layout();
}


void
FileSelectionPage::_Layout()
{
	LayoutDescriptionVertically(fDescription);

	float left = fFile->Frame().left;
	float top = fDescription->Frame().bottom + kTextDistance;

	// center "file" text field and "select" button vertically
	float selectTop = top;
	float fileTop = top;
	float fileHeight = fFile->Bounds().Height();
	float selectHeight = fSelect->Bounds().Height();
	if (fileHeight < selectHeight) {
		int delta = (int)((selectHeight - fileHeight + 1) / 2);
		fileTop += delta;
	} else {
		int delta = (int)((fileHeight - selectHeight + 1) / 2);
		selectTop += delta;
	}

	fFile->MoveTo(left, fileTop);

	float width = fSelect->Frame().left - kFileButtonDistance - left;
	fFile->ResizeTo(width, fileHeight);

	left = fSelect->Frame().left;
	fSelect->MoveTo(left, selectTop);
}


void
FileSelectionPage::_OpenFilePanel()
{
	if (fFilePanel != NULL)
		return;

	const entry_ref* directory = NULL;
	entry_ref base;
	BPath file(fFile->Text());
	BPath parent;

	if (file.GetParent(&parent) == B_OK &&
		get_ref_for_path(parent.Path(), &base) == B_OK)
		directory = &base;

	BMessenger messenger(this);
	fFilePanel = new BFilePanel(fMode, &messenger, directory,
		B_FILE_NODE,
		false,
		NULL,
		NULL,
		true);
	if (fMode == B_SAVE_PANEL && file.Leaf() != NULL)
		fFilePanel->SetSaveText(file.Leaf());
	fFilePanel->Show();
}


void
FileSelectionPage::_SetFileFromFilePanelMessage(BMessage* message)
{
	if (message->what == B_SAVE_REQUESTED) {
		entry_ref directory;
		BString name;
		message->FindRef("directory", &directory);
		message->FindString("name", &name);
		BPath path(&directory);
		if (path.Append(name.String()) == B_OK)
			fFile->SetText(path.Path());
	} else {
		entry_ref entryRef;
		message->FindRef("refs", &entryRef);
		BEntry entry(&entryRef);
		BPath path;
		if (entry.GetPath(&path) == B_OK)
			fFile->SetText(path.Path());
	}
}


void
FileSelectionPage::_FilePanelCanceled()
{
	delete fFilePanel;
	fFilePanel = NULL;
}
