/*
 * Copyright 2006-2011, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <stdio.h>

#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Screen.h>
#include <TextControl.h>

#include "NamePanel.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "LaunchBox"


enum {
	MSG_PANEL_OK,
	MSG_PANEL_CANCEL,
};


NamePanel::NamePanel(const char* label, const char* text, BWindow* window,
		BHandler* target, BMessage* message, const BSize& size)
	:
	Panel(BRect(B_ORIGIN, size), B_TRANSLATE("Name Panel"),
		B_MODAL_WINDOW_LOOK, B_MODAL_SUBSET_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_V_RESIZABLE
			| B_AUTO_UPDATE_SIZE_LIMITS),
	fWindow(window),
	fTarget(target),
	fMessage(message)
{
	BButton* defaultButton = new BButton(B_TRANSLATE("OK"),
		new BMessage(MSG_PANEL_OK));
	BButton* cancelButton = new BButton(B_TRANSLATE("Cancel"),
		new BMessage(MSG_PANEL_CANCEL));
	fNameTC = new BTextControl(label, text, NULL);
	BLayoutItem* inputItem = fNameTC->CreateTextViewLayoutItem();
	inputItem->SetExplicitMinSize(
		BSize(fNameTC->StringWidth("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"),
			B_SIZE_UNSET));

	BLayoutBuilder::Group<>(this, B_VERTICAL, 10)
		.AddGlue()

		// controls
		.AddGroup(B_HORIZONTAL, 5)
			.AddStrut(5)

			// text control
			.Add(fNameTC->CreateLabelLayoutItem())
			.Add(inputItem)
			.AddStrut(5)
			.End()

		.AddGlue()

		// buttons
		.AddGroup(B_HORIZONTAL, 5)
			.AddGlue()
			.Add(cancelButton)
			.Add(defaultButton)
			.AddStrut(5)
			.End()

		.AddGlue();

	SetDefaultButton(defaultButton);
	fNameTC->MakeFocus(true);

	if (fWindow && fWindow->Lock()) {
		fSavedTargetWindowFeel = fWindow->Feel();
		if (fSavedTargetWindowFeel != B_NORMAL_WINDOW_FEEL)
			fWindow->SetFeel(B_NORMAL_WINDOW_FEEL);
		fWindow->Unlock();
	}	

	AddToSubset(fWindow);
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
