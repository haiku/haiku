/*
 * Copyright 2002-2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 */

#include "ColorPickerPanel.h"

#include <stdio.h>

#include <Autolock.h>
#include <Application.h>
#include <Locker.h>

#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <GroupView.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <SeparatorView.h>

#include "support_ui.h"

#include "ColorPickerView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-ColorPicker"


enum {
	MSG_CANCEL					= 'cncl',
	MSG_DONE					= 'done',
};


ColorPickerPanel::ColorPickerPanel(BRect frame, rgb_color color,
								   SelectedColorMode mode,
								   BWindow* window,
								   BMessage* message, BHandler* target)
	: Panel(frame, "Pick a color",
			B_FLOATING_WINDOW_LOOK, B_FLOATING_SUBSET_WINDOW_FEEL,
			B_ASYNCHRONOUS_CONTROLS |
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_CLOSABLE
			| B_AUTO_UPDATE_SIZE_LIMITS),
	  fWindow(window),
	  fMessage(message),
	  fTarget(target)
{
	SetTitle(B_TRANSLATE("Pick a color"));

	fColorPickerView = new ColorPickerView("color picker", color, mode);

	SetLayout(new BGroupLayout(B_VERTICAL));
		
	BButton* defaultButton = new BButton("ok button",
		B_TRANSLATE("OK"),
		new BMessage(MSG_DONE));
	BButton* cancelButton = new BButton("cancel button",
		B_TRANSLATE("Cancel"),
		new BMessage(MSG_CANCEL));

	BView* topView = new BGroupView(B_VERTICAL);

	const float inset = be_control_look->DefaultLabelSpacing();

	BLayoutBuilder::Group<>(topView, B_VERTICAL, 0.0f)
		.Add(fColorPickerView)
		.Add(new BSeparatorView(B_HORIZONTAL))
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(cancelButton)
			.Add(defaultButton)
			.SetInsets(inset, inset, inset, inset)
		.End()
	;

	SetDefaultButton(defaultButton);

	if (fWindow != NULL)
		AddToSubset(fWindow);
	else
		SetFeel(B_FLOATING_APP_WINDOW_FEEL);

	AddChild(topView);
}


ColorPickerPanel::~ColorPickerPanel()
{
	delete fMessage;
}


void
ColorPickerPanel::Cancel()
{
	PostMessage(MSG_CANCEL);
}


void
ColorPickerPanel::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_CANCEL:
		case MSG_DONE:
		{
			BMessage msg('PSTE');
			BLooper* looper = fTarget ? fTarget->Looper() : be_app;
			if (fMessage)
				msg = *fMessage;
			if (message->what == MSG_DONE)
				store_color_in_message(&msg, fColorPickerView->Color());
			msg.AddRect("panel frame", Frame());
			msg.AddInt32("panel mode", fColorPickerView->Mode());
			msg.AddBool("begin", true);
			looper->PostMessage(&msg, fTarget);
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		default:
			Panel::MessageReceived(message);
			break;
	}
}


// #pragma mark -


void
ColorPickerPanel::SetColor(rgb_color color)
{
	fColorPickerView->SetColor(color);
}


void
ColorPickerPanel::SetMessage(BMessage* message)
{
	if (message != fMessage) {
		delete fMessage;
		fMessage = message;
	}
}


void
ColorPickerPanel::SetTarget(BHandler* target)
{
	fTarget = target;
}

