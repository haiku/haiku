/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Rafael Romo
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "RefreshWindow.h"

#include "Constants.h"
#include "RefreshSlider.h"

#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <Catalog.h>
#include <String.h>
#include <StringView.h>
#include <Window.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Screen"


RefreshWindow::RefreshWindow(BPoint position, float current, float min, float max)
	: BWindow(BRect(0, 0, 300, 200), B_TRANSLATE("Refresh rate"), B_MODAL_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS, B_ALL_WORKSPACES)
{
	min = ceilf(min);
	max = floorf(max);

	BView* topView = new BView(Bounds(), NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	BRect rect = Bounds().InsetByCopy(8, 8);
	BStringView* stringView = new BStringView(rect, "info",
		B_TRANSLATE("Type or use the left and right arrow keys."));
	stringView->ResizeToPreferred();
	topView->AddChild(stringView);

	rect.top += stringView->Bounds().Height() + 14;
	fRefreshSlider = new RefreshSlider(rect, min, max, B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT);
	fRefreshSlider->SetValue((int32)rintf(current * 10));
	fRefreshSlider->SetModificationMessage(new BMessage(SLIDER_MODIFICATION_MSG));
	float width, height;
	fRefreshSlider->GetPreferredSize(&width, &height);
	fRefreshSlider->ResizeTo(rect.Width(), height);
	topView->AddChild(fRefreshSlider);

	BButton* doneButton = new BButton(rect, "DoneButton", B_TRANSLATE("Done"), 
		new BMessage(BUTTON_DONE_MSG), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	doneButton->ResizeToPreferred();
	doneButton->MoveTo(Bounds().Width() - doneButton->Bounds().Width() - 8,
		Bounds().Height() - doneButton->Bounds().Height() - 8);
	topView->AddChild(doneButton);

	BButton* button = new BButton(doneButton->Frame(), "CancelButton",
		B_TRANSLATE("Cancel"), new BMessage(B_QUIT_REQUESTED),
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	button->ResizeToPreferred();
	button->MoveBy(-button->Bounds().Width() - 10, 0);
	topView->AddChild(button);

	doneButton->MakeDefault(true);

	width = stringView->Bounds().Width() + 100;
	if (width < Bounds().Width())
		width = Bounds().Width();
	height = fRefreshSlider->Frame().bottom + button->Bounds().Height() + 20.0f;

	ResizeTo(width, height);
	MoveTo(position.x - width / 2.5f, position.y - height / 1.9f);
}


void
RefreshWindow::WindowActivated(bool active)
{
	fRefreshSlider->MakeFocus(active);
}


void
RefreshWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case BUTTON_DONE_MSG:
		{
			float value = (float)fRefreshSlider->Value() / 10;

			BMessage message(SET_CUSTOM_REFRESH_MSG);
			message.AddFloat("refresh", value);
			be_app->PostMessage(&message);

			PostMessage(B_QUIT_REQUESTED);
			break;
		}

		case SLIDER_INVOKE_MSG:
			fRefreshSlider->MakeFocus(true);
			break;

		default:
			BWindow::MessageReceived(message);		
			break;
	}
}
