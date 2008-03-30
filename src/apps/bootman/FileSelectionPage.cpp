/*
 * Copyright 2008, Michael Pfeiffer, laplace@users.sourceforge.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "FileSelectionPage.h"


#include <Button.h>
#include <RadioButton.h>
#include <TextControl.h>
#include <TextView.h>

#include <string.h>
#include <String.h>


FileSelectionPage::FileSelectionPage(BMessage* settings, BRect frame, const char* name,
		const char* description)
	: WizardPageView(settings, frame, name, B_FOLLOW_ALL, 
		B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE)
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
	
	// TODO open file dialog
	fSelect = new BButton(rect, "select", "Select", new BMessage(),
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

