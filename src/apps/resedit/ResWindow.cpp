/*
 * Copyright (c) 2005-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@gmail.com>
 */
#include "ResWindow.h"

#include "App.h"
#include "ResView.h"

#include <Alert.h>

static int32 sWindowCount = 0;

ResWindow::ResWindow(const BRect &rect, const entry_ref *ref)
 :	BWindow(rect, "", B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	atomic_add(&sWindowCount, 1);
	
	fView = new ResView(Bounds(), "resview", B_FOLLOW_ALL, B_WILL_DRAW, ref);
	AddChild(fView);
	Show();
}


ResWindow::~ResWindow(void)
{
}


bool
ResWindow::QuitRequested(void)
{
	if (fView->GetSaveStatus() == FILE_DIRTY) {
		BAlert *alert = new BAlert("ResEdit", "Save changes before closing?",
			"Cancel", "Don't save", "Save",
			B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
		alert->SetShortcut(0, B_ESCAPE);
		alert->SetShortcut(1, 'd');
		alert->SetShortcut(2, 's');

		switch (alert->Go()) {
			case 0:
				return false;
			case 2: {
				fView->SaveAndQuit();
				return false;
			}
			default:
				break;
		}
	}
	
	atomic_add(&sWindowCount, -1);
	
	if (sWindowCount == 0)
		be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


