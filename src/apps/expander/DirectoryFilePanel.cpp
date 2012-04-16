/*
 * Copyright 2004-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */


#include "DirectoryFilePanel.h"

#include <Catalog.h>
#include <Locale.h>
#include <Window.h>

#include <stdio.h>
#include <string.h>

#include <compat/sys/stat.h>


DirectoryRefFilter::DirectoryRefFilter()
{
}


bool
DirectoryRefFilter::Filter(const entry_ref *ref, BNode* node,
	struct stat_beos *st, const char *filetype)
{
	if (S_ISDIR(st->st_mode))
		return true;

	if (S_ISLNK(st->st_mode)) {
		// Traverse symlinks
		BEntry entry(ref, true);
		return entry.IsDirectory();
	}

	return false;
}


//	#pragma mark -

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DirectoryFilePanel"


DirectoryFilePanel::DirectoryFilePanel(file_panel_mode mode, BMessenger* target,
		const entry_ref* startDirectory, uint32 nodeFlavors,
		bool allowMultipleSelection, BMessage* message, BRefFilter* filter,
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

		rect.right = rect.left -= 30;
		float width = be_plain_font->StringWidth(
			B_TRANSLATE("Select current")) + 20;
		rect.left = width > 75 ? rect.right - width : rect.right - 75;
		fCurrentButton = new BButton(rect, "directoryButton",
			B_TRANSLATE("Select current"), new BMessage(MSG_DIRECTORY),
			B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);

		background->AddChild(fCurrentButton);
		fCurrentButton->SetTarget(Messenger());

		SetButtonLabel(B_DEFAULT_BUTTON, B_TRANSLATE("Select"));
		Window()->SetTitle(B_TRANSLATE("Expander: Choose destination"));

		Window()->Unlock();

		SelectionChanged();
	}

	BFilePanel::Show();
}


void
DirectoryFilePanel::SelectionChanged()
{
	Window()->Lock();

	char label[64];
	entry_ref ref;
	GetPanelDirectory(&ref);
	if (snprintf(label, sizeof(label),
		B_TRANSLATE("Select '%s'"), ref.name) >= (int)sizeof(label))
			strcpy(label + sizeof(label) - 5, B_UTF8_ELLIPSIS "'");

	// Resize button so that the label fits
	// maximum width is dictated by the window's size limits

	float dummy, maxWidth;
	Window()->GetSizeLimits(&maxWidth, &dummy, &dummy, &dummy);
	maxWidth -= Window()->Bounds().Width() + 8 - fCurrentButton->Frame().right;

	BRect oldBounds = fCurrentButton->Bounds();
	fCurrentButton->SetLabel(label);
	float width, height;
	fCurrentButton->GetPreferredSize(&width, &height);
	if (width > maxWidth)
		width = maxWidth;
	fCurrentButton->ResizeTo(width, oldBounds.Height());
	fCurrentButton->MoveBy(oldBounds.Width() - width, 0);

	Window()->Unlock();

	BFilePanel::SelectionChanged();
}
