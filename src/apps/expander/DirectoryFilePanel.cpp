/*
 * Copyright 2004-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */


#include "DirectoryFilePanel.h"

#include <Window.h>

#include <stdio.h>
#include <string.h>


DirectoryRefFilter::DirectoryRefFilter()
	: BRefFilter()
{
}


bool
DirectoryRefFilter::Filter(const entry_ref *ref, BNode* node, struct stat *st, 
	const char *filetype)
{
	return node->IsDirectory();
}


//	#pragma mark -


DirectoryFilePanel::DirectoryFilePanel(file_panel_mode mode, BMessenger *target, 
	const entry_ref *startDirectory, uint32 nodeFlavors, 
	bool allowMultipleSelection, BMessage *message, BRefFilter *filter,	
	bool modal,	bool hideWhenDone)
	: BFilePanel(mode, target, startDirectory, nodeFlavors, 
		allowMultipleSelection, message, filter, modal, hideWhenDone),
		fCurrentButton(NULL)
{
}


void
DirectoryFilePanel::Show()
{
	if (fCurrentButton == NULL) {
		Window()->Lock();
		BView* background = Window()->ChildAt(0); 
		BView* cancel = background->FindView("cancel button");

		BRect rect;
		if (cancel != NULL)
			rect = cancel->Frame();
		else {
			rect = background->Bounds();
			rect.left = rect.right;
			rect.top = rect.bottom - 35;
			rect.bottom -= 10;
		}

		fCurrentButton = new BButton(rect, "directoryButton", "Select current",
			new BMessage(MSG_DIRECTORY), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
		fCurrentButton->ResizeToPreferred();
		fCurrentButton->MoveBy(-fCurrentButton->Bounds().Width() - 8, 0);
		background->AddChild(fCurrentButton);
		fCurrentButton->SetTarget(Messenger());

		SetButtonLabel(B_DEFAULT_BUTTON, "Select");
		Window()->Unlock();

		SelectionChanged();
	}

	BFilePanel::Show();
}


void
DirectoryFilePanel::SelectionChanged(void)
{
	Window()->Lock();

	char label[64];
	entry_ref ref;
	GetPanelDirectory(&ref);
	if (snprintf(label, sizeof(label), "Select '%s'", ref.name) >= (int)sizeof(label))
		strcpy(label + sizeof(label) - 5, B_UTF8_ELLIPSIS "'");

	// Resize button so that the label fits
	// maximum width is dictated by the window's size limits

	float dummy, maxWidth;
	Window()->GetSizeLimits(&maxWidth, &dummy, &dummy, &dummy);
	maxWidth -= Window()->Bounds().Width() + 8 - fCurrentButton->Frame().right;

	float oldWidth = fCurrentButton->Bounds().Width();
	fCurrentButton->SetLabel(label);
	float width, height;
	fCurrentButton->GetPreferredSize(&width, &height);
	if (width > maxWidth)
		width = maxWidth;
	fCurrentButton->ResizeTo(width, height);
	fCurrentButton->MoveBy(oldWidth - width, 0);

	Window()->Unlock();

	BFilePanel::SelectionChanged();
}
