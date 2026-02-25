/*
 * Copyright 2026 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include "LabelWindow.h"

#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <TextControl.h>

#include "MailSupport.h"
#include "Messages.h"



#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "LabelWindow"


enum _messages {
	kLabel = 128,
	kOK,
	kCancel
};


TLabelWindow::TLabelWindow(BRect frame, BMessenger target)
	: BWindow(BRect(), "", B_MODAL_WINDOW, B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fTarget(target)
{
	fLabel = new BTextControl("label",  B_TRANSLATE("New label:"), "", NULL);
	fLabel->SetModificationMessage(new BMessage(kLabel));
	// disallow control characters and "/"
	for (uint32 i = 0; i < 0x20; ++i)
		fLabel->TextView()->DisallowChar(i);
	fLabel->TextView()->DisallowChar('/');

	fOK = new BButton("ok", B_TRANSLATE("OK"), new BMessage(kOK));
	fOK->MakeDefault(true);
	fOK->SetEnabled(false);

	BButton *cancel = new BButton("cancel", B_TRANSLATE("Cancel"), new BMessage(kCancel));

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL, 0)
			.Add(fLabel)
		.End()
		.AddStrut(B_USE_SMALL_SPACING)
		.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING, 0)
			.AddStrut(B_USE_SMALL_SPACING)
			.Add(cancel)
			.Add(fOK);

	fLabel->BTextControl::MakeFocus(true);
	CenterIn(frame);
	Show();
}


TLabelWindow::~TLabelWindow()
{
}


void
TLabelWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kLabel:
			fOK->SetEnabled(true);
			break;

		case kOK:
		{
			const char* newLabel = fLabel->Text();
			if (create_label_file(newLabel) == B_OK) {
				BMessage message(M_SET_LABEL);
				message.AddString("label", newLabel);
				fTarget.SendMessage(&message);
			}
			// intentional fall-through
		}
		case kCancel:
			Quit();
			break;
	}
}
