/*
 * Copyright 2019-2023, Adrien Destugues, pulkomandy@pulkomandy.tk.
 * Distributed under the terms of the MIT License.
 */


#include "AlertWithCheckbox.h"

#include <algorithm>
#include <stdio.h>

#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <IconUtils.h>
#include <Resources.h>
#include <StripeView.h>
#include <TextView.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DebugServer"


enum {
	kButton1Message,
	kButton2Message,
	kButton3Message
};


AlertWithCheckbox::AlertWithCheckbox(const char* title, const char* messageText,
	const char* checkboxLabel, const char* label1, const char* label2, const char* label3)
	:
	BWindow(BRect(0, 0, 100, 50), title,
		B_MODAL_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL,
		B_CLOSE_ON_ESCAPE | B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fBitmap(IconSize(), B_RGBA32),
	fSemaphore(create_sem(0, "AlertWithCheckbox")),
	fAction(0)
{
	BResources resources;
	resources.SetToImage(B_TRANSLATION_CONTEXT);
	size_t size;
	const uint8* iconData = (const uint8*)resources.LoadResource('VICN', 1,
		&size);

	// TODO load "info" icon from app_server instead?
	BIconUtils::GetVectorIcon(iconData, size, &fBitmap);
	BStripeView *stripeView = new BStripeView(fBitmap);

	BTextView *message = new BTextView("_tv_");
	message->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	rgb_color textColor = ui_color(B_PANEL_TEXT_COLOR);
	message->SetFontAndColor(be_plain_font, B_FONT_ALL, &textColor);
	message->MakeEditable(false);
	message->MakeSelectable(false);
	message->SetText(messageText);
	BRect textRect = message->TextRect();
	textRect.PrintToStream();
	message->SetWordWrap(true);
	message->SetExplicitMaxSize(BSize(B_SIZE_UNSET, B_SIZE_UNSET));
	float width = message->StringWidth("W") * 40;
	if (width < textRect.Width()) {
		message->SetExplicitMinSize(BSize(width, B_SIZE_UNSET));
	}

	fDontAskAgain = new BCheckBox("checkbox",
		checkboxLabel, NULL);

	BButton* button1 = new BButton(label1, label1, new BMessage(kButton1Message));
	BButton* button2 = new BButton(label2, label2, new BMessage(kButton2Message));
	BButton* button3 = new BButton(label3, label3, new BMessage(kButton3Message));
	button1->MakeDefault(true);

	BLayoutBuilder::Group<>(this)
		.AddGroup(B_HORIZONTAL)
			.Add(stripeView)
			.AddGroup(B_VERTICAL)
				.SetInsets(B_USE_SMALL_SPACING)
				.Add(message)
				.AddGroup(B_HORIZONTAL, 0)
					.Add(fDontAskAgain)
					.AddGlue()
				.End()
				.AddGroup(B_HORIZONTAL)
					.AddGlue()
					.Add(button1)
					.Add(button2)
					.Add(button3)
				.End()
			.End()
		.End();

	ResizeToPreferred();
	CenterOnScreen();
}


AlertWithCheckbox::~AlertWithCheckbox()
{
	delete_sem(fSemaphore);
}


void
AlertWithCheckbox::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case B_QUIT_REQUESTED:
			release_sem(fSemaphore);
			break;

		case 0:
		case 1:
		case 2:
			fAction = message->what;
			PostMessage(B_QUIT_REQUESTED);
			return;
	}

	BWindow::MessageReceived(message);
}


int32
AlertWithCheckbox::Go(bool& dontAskAgain)
{
	Show();

	// Wait for user to close the window
	acquire_sem(fSemaphore);
	dontAskAgain = fDontAskAgain->Value();
	return fAction;
}


BRect
AlertWithCheckbox::IconSize()
{
	return BRect(BPoint(0, 0), be_control_look->ComposeIconSize(32));
}
