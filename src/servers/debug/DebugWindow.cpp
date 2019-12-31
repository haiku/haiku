/*
 * Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk.
 * Distributed under the terms of the MIT License.
 */


#include "DebugWindow.h"

#include <algorithm>
#include <stdio.h>

#include <Button.h>
#include <Catalog.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <IconUtils.h>
#include <Resources.h>
#include <RadioButton.h>
#include <StripeView.h>
#include <TextView.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DebugServer"


DebugWindow::DebugWindow(const char* appName)
	:
	BWindow(BRect(0, 0, 100, 50), "Crashed program", B_MODAL_WINDOW,
		B_CLOSE_ON_ESCAPE | B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fBitmap(IconSize(), B_RGBA32),
	fSemaphore(create_sem(0, "DebugWindow")),
	fAction(kActionKillTeam)
{
	BString buffer(B_TRANSLATE(
		"The application:\n\n      %app\n\n"
		"has encountered an error which prevents it from continuing. Haiku "
		"will terminate the application and clean up."));
	buffer.ReplaceFirst("%app", appName);

	BResources resources;
	resources.SetToImage(B_TRANSLATION_CONTEXT);
	size_t size;
	const uint8* iconData = (const uint8*)resources.LoadResource('VICN', 2,
		&size);
	BIconUtils::GetVectorIcon(iconData, size, &fBitmap);
	BStripeView *stripeView = new BStripeView(fBitmap);

	BTextView *message = new BTextView("_tv_");
	message->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	rgb_color textColor = ui_color(B_PANEL_TEXT_COLOR);
	message->SetFontAndColor(be_plain_font, B_FONT_ALL, &textColor);
	message->MakeEditable(false);
	message->MakeSelectable(false);
	message->SetWordWrap(true);
	message->SetText(buffer);
	message->SetExplicitMaxSize(BSize(B_SIZE_UNSET, B_SIZE_UNSET));
	float width = message->StringWidth(appName)
		+ message->StringWidth(" ") * 12;
	width = std::max(width, message->StringWidth("W") * 30);
	message->SetExplicitMinSize(BSize(width, B_SIZE_UNSET));

	BRadioButton *terminate = new BRadioButton("terminate",
		B_TRANSLATE("Terminate"), new BMessage(kActionKillTeam));
#ifdef HANDOVER_USE_DEBUGGER
	BRadioButton *report = new BRadioButton("report",
		B_TRANSLATE("Save report"), new BMessage(kActionSaveReportTeam));
#endif
	BRadioButton *debug = new BRadioButton("debug",
		B_TRANSLATE("Debug"), new BMessage(kActionDebugTeam));
	BRadioButton *core = new BRadioButton("core",
		B_TRANSLATE("Write core file"), new BMessage(kActionWriteCoreFile));

	fOKButton = new BButton("close", B_TRANSLATE("Oh no!"),
		new BMessage(B_QUIT_REQUESTED));
	fOKButton->MakeDefault(true);

	terminate->SetValue(B_CONTROL_ON);

	BLayoutBuilder::Group<>(this)
		.AddGroup(B_HORIZONTAL)
			.Add(stripeView)
			.AddGroup(B_VERTICAL)
				.SetInsets(B_USE_SMALL_SPACING)
				.Add(message)
				.AddGroup(B_VERTICAL, 0)
					.Add(terminate)
					.Add(debug)
					.Add(report)
					.Add(core)
				.End()
				.AddGroup(B_HORIZONTAL)
					.AddGlue()
					.Add(fOKButton)
				.End()
			.End()
		.End();

	ResizeToPreferred();
	CenterOnScreen();
}


DebugWindow::~DebugWindow()
{
	delete_sem(fSemaphore);
}


void
DebugWindow::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case B_QUIT_REQUESTED:
			release_sem(fSemaphore);
			break;

		case kActionKillTeam:
		case kActionDebugTeam:
		case kActionWriteCoreFile:
		case kActionSaveReportTeam:
			fAction = message->what;
			fOKButton->SetLabel(fAction == kActionDebugTeam
				? B_TRANSLATE("Oh yeah!") : B_TRANSLATE("Oh no!"));
			return;
	}

	BWindow::MessageReceived(message);
}


int32
DebugWindow::Go()
{
	Show();

	// Wait for user to close the window
	acquire_sem(fSemaphore);
	return fAction;
}


BRect
DebugWindow::IconSize()
{
	int32 size = std::max((int32)1, ((int32)be_plain_font->Size() + 15) / 16)
		* 32 - 1;
	return BRect(0, 0, size, size);
}
