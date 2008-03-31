/*
 * Copyright 2008, Michael Pfeiffer, laplace@users.sourceforge.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "FileSelectionPage.h"


#include <Button.h>
#include <Path.h>
#include <RadioButton.h>
#include <TextControl.h>
#include <TextView.h>

#include <string.h>
#include <String.h>


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
	
	// TODO align text and button 
	fFile = new BTextControl(rect, "file", "File:", file.String(), new BMessage());
	fFile->SetDivider(be_plain_font->StringWidth(fFile->Label()) + 5);
	AddChild(fFile);
	
	fSelect = new BButton(rect, "select", "Select", new BMessage(kMsgOpenFilePanel),
		B_FOLLOW_RIGHT);
	fSelect->ResizeToPreferred();
	float left = rect.right - fSelect->Frame().Width();
	fSelect->MoveTo(left, 0);
	AddChild(fSelect);
	
	_Layout();
}


void
FileSelectionPage::_Layout()
{
	LayoutDescriptionVertically(fDescription);
	
	float left = fFile->Frame().left;
	float top = fDescription->Frame().bottom + kTextDistance;
	fFile->MoveTo(left, top);
	
	float width = fSelect->Frame().left - kFileButtonDistance - left;
	float height = fFile->Frame().Height();
	fFile->ResizeTo(width, height);
	
	left = fSelect->Frame().left;
	fSelect->MoveTo(left, top);
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
