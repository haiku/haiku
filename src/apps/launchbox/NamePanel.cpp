/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <stdio.h>

#include <Button.h>
#include <GroupLayoutBuilder.h>
#include <SpaceLayoutItem.h>
#include <Screen.h>
#include <TextControl.h>

#include "NamePanel.h"

enum {
	MSG_PANEL_OK,
	MSG_PANEL_CANCEL,
};

// constructor
NamePanel::NamePanel(const char* label, const char* text, BWindow* window,
		BHandler* target, BMessage* message, BRect frame)
	:
	Panel(frame, "Name Panel",
		B_MODAL_WINDOW_LOOK, B_MODAL_SUBSET_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_V_RESIZABLE
			| B_AUTO_UPDATE_SIZE_LIMITS),
	fWindow(window),
	fTarget(target),
	fMessage(message)
{
	BButton* defaultButton = new BButton("OK", new BMessage(MSG_PANEL_OK));
	BButton* cancelButton = new BButton("Cancel",
		new BMessage(MSG_PANEL_CANCEL));
	fNameTC = new BTextControl(label, text, NULL);

	BView* topView = BGroupLayoutBuilder(B_VERTICAL, 10)
		.AddGlue()

		// controls
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 5)
			.Add(BSpaceLayoutItem::CreateHorizontalStrut(5))

			// text control
			.Add(fNameTC->CreateLabelLayoutItem())
			.Add(fNameTC->CreateTextViewLayoutItem())

			.Add(BSpaceLayoutItem::CreateHorizontalStrut(5))
		)

		.AddGlue()

		// buttons
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 5)
			.Add(BSpaceLayoutItem::CreateGlue())
			.Add(cancelButton)
			.Add(defaultButton)
			.Add(BSpaceLayoutItem::CreateHorizontalStrut(5))
		)

		.AddGlue()
	;

	SetLayout(new BGroupLayout(B_HORIZONTAL));	
	AddChild(topView);

	SetDefaultButton(defaultButton);
	fNameTC->MakeFocus(true);

	if (fWindow && fWindow->Lock()) {
		fSavedTargetWindowFeel = fWindow->Feel();
		if (fSavedTargetWindowFeel != B_NORMAL_WINDOW_FEEL)
			fWindow->SetFeel(B_NORMAL_WINDOW_FEEL);
		fWindow->Unlock();
	}	

	AddToSubset(fWindow);

	if (!frame.IsValid())
		CenterOnScreen();

	Show();
}


NamePanel::~NamePanel()
{
	if (fWindow && fWindow->Lock()) {
		fWindow->SetFeel(fSavedTargetWindowFeel);
		fWindow->Unlock();
	}	
	delete fMessage;
}


void NamePanel::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_PANEL_CANCEL:
			Quit();
			break;
		case MSG_PANEL_OK: {
			if (!fTarget)
				fTarget = fWindow;
			BLooper* looper = fTarget ? fTarget->Looper() : NULL;
			if (fMessage && looper) {
				BMessage cloneMessage(*fMessage);
				cloneMessage.AddString("name", fNameTC->Text());
				cloneMessage.AddRect("frame", Frame());
				looper->PostMessage(&cloneMessage, fTarget);
			}
			Quit();
			break;
		}
		default:
			Panel::MessageReceived(message);
	}
}
