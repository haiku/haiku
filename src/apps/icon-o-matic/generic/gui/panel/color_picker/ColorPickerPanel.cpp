/* 
 * Copyright 2002-2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 *		
 */

#include "ColorPickerPanel.h"

#include <stdio.h>

#include <Application.h>
#include <Catalog.h>
#include <Locale.h>

#if LIB_LAYOUT
#  include <MBorder.h>
#  include <HGroup.h>
#  include <Space.h>
#  include <MButton.h>
#  include <VGroup.h>
#else
#  include <Box.h>
#  include <Button.h>
#endif

#include "support_ui.h"

#include "ColorPickerView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-ColorPicker"


enum {
	MSG_CANCEL					= 'cncl',
	MSG_DONE					= 'done',
};

// constructor
ColorPickerPanel::ColorPickerPanel(BRect frame, rgb_color color,
								   selected_color_mode mode,
								   BWindow* window,
								   BMessage* message, BHandler* target)
	: Panel(frame, "Pick Color",
			B_FLOATING_WINDOW_LOOK, B_FLOATING_SUBSET_WINDOW_FEEL,
			B_ASYNCHRONOUS_CONTROLS |
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_CLOSABLE),
	  fWindow(window),
	  fMessage(message),
	  fTarget(target)
{
	SetTitle(B_TRANSLATE("Pick a color"));

	fColorPickerView = new ColorPickerView("color picker", color, mode);

#if LIB_LAYOUT
	MButton* defaultButton = new MButton(B_TRANSLATE("OK"),
		new BMessage(MSG_DONE), this);

	// interface layout
	BView* topView = new VGroup (
		fColorPickerView,
		new MBorder (
			M_RAISED_BORDER, 5, "buttons",
			new HGroup (
				new Space(minimax(0.0, 0.0, 10000.0, 10000.0, 5.0)),
				new MButton(B_TRANSLATE("Cancel"), new BMessage(MSG_CANCEL), 
					this),
				new Space(minimax(5.0, 0.0, 10.0, 10000.0, 1.0)),
				defaultButton,
				new Space(minimax(2.0, 0.0, 2.0, 10000.0, 0.0)),
				0
			)
		),
		0
	);
#else // LIB_LAYOUT
	frame = BRect(0, 0, 40, 15);
	BButton* defaultButton = new BButton(frame, "ok button", 
								B_TRANSLATE("OK"), new BMessage(MSG_DONE),
								B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	defaultButton->ResizeToPreferred();
	BButton* cancelButton = new BButton(frame, "cancel button", 
								B_TRANSLATE("Cancel"), new BMessage(MSG_CANCEL),
								B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	cancelButton->ResizeToPreferred();

	frame.bottom = frame.top + (defaultButton->Frame().Height() + 16);
	frame.right = frame.left + fColorPickerView->Frame().Width();
	BBox* buttonBox = new BBox(frame, "button group",
							   B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM,
							   B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
							   B_PLAIN_BORDER);

	ResizeTo(frame.Width(),
			 fColorPickerView->Frame().Height() + frame.Height() + 1);

	frame = Bounds();
	BView* topView = new BView(frame, "bg", B_FOLLOW_ALL, 0);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	buttonBox->MoveTo(frame.left, frame.bottom - buttonBox->Frame().Height());

	defaultButton->MoveTo(frame.right - defaultButton->Frame().Width() - 10,
						  frame.top + 8);
	buttonBox->AddChild(defaultButton);

	cancelButton->MoveTo(defaultButton->Frame().left - 10
							- cancelButton->Frame().Width(),
						 frame.top + 8);
	buttonBox->AddChild(cancelButton);

	topView->AddChild(fColorPickerView);
	topView->AddChild(buttonBox);
#endif // LIB_LAYOUT

	SetDefaultButton(defaultButton);

	if (fWindow)
		AddToSubset(fWindow);
	else
		SetFeel(B_FLOATING_APP_WINDOW_FEEL);

	AddChild(topView);
}

// destructor
ColorPickerPanel::~ColorPickerPanel()
{
	delete fMessage;
}

// Cancel
void
ColorPickerPanel::Cancel()
{
	PostMessage(MSG_CANCEL);
}

// MessageReceived
void
ColorPickerPanel::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_CANCEL:
		case MSG_DONE: {
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

// SetColor
void
ColorPickerPanel::SetColor(rgb_color color)
{
	fColorPickerView->SetColor(color);
}

// SetMessage
void
ColorPickerPanel::SetMessage(BMessage* message)
{
	delete fMessage;
	fMessage = message;
}

// SetTarget
void
ColorPickerPanel::SetTarget(BHandler* target)
{
	fTarget = target;
}
