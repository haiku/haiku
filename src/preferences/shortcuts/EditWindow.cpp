/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Josef Gajdusek
 */


#include "EditWindow.h"

#include <math.h>

#include <Button.h>
#include <LayoutBuilder.h>
#include <TextControl.h>
#include <String.h>
#include <StringView.h>

#include "ShortcutsWindow.h"


EditWindow::EditWindow(const char* placeholder, uint32 flags)
	:
	BWindow(BRect(0, 0, 0, 0), "", B_MODAL_WINDOW, flags)
{
	fTextControl = new BTextControl("", placeholder, NULL);

	BButton* okButton = new BButton("OK", new BMessage(B_CONTROL_MODIFIED));
	okButton->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_TOP));
	SetDefaultButton(okButton);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(fTextControl)
		.Add(okButton);
}


void
EditWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_CONTROL_MODIFIED:
			delete_sem(fSem);
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


BString
EditWindow::Go()
{
	fSem = create_sem(0, "EditSem");
	if (fSem < B_OK) {
		Quit();
		return "";
	}

	BSize psize = GetLayout()->PreferredSize();
	ResizeTo(max_c(be_plain_font->StringWidth(fTextControl->Text()) * 1.5,
				psize.Width()),
		psize.Height());
	Show();
	CenterOnScreen();

	acquire_sem(fSem);
	BString result = fTextControl->Text();
	if (Lock())
		Quit();

	return result;
}
