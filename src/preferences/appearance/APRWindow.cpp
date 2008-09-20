/*
 * Copyright 2002-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm (darkwyrm@earthlink.net)
 */

#include <Button.h>
#include <Messenger.h>
#include <TabView.h>
#include "APRWindow.h"
#include "APRView.h"
#include "defs.h"

static const uint32 kMsgSetDefaults = 'dflt';
static const uint32 kMsgRevert = 'rvrt';

APRWindow::APRWindow(BRect frame)
 :	BWindow(frame, "Appearance", B_TITLED_WINDOW, B_NOT_ZOOMABLE,
 			B_ALL_WORKSPACES)
{
	BRect rect = Bounds();
	BView* view = new BView(rect, "background", B_FOLLOW_ALL, B_WILL_DRAW);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);

	rect.left = 10;
	rect.top = rect.bottom - 10;
	fDefaultsButton = new BButton(rect, "defaults", "Defaults",
		new BMessage(kMsgSetDefaults), B_FOLLOW_LEFT
			| B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fDefaultsButton->ResizeToPreferred();
	fDefaultsButton->SetEnabled(false);
	float buttonHeight = fDefaultsButton->Bounds().Height();
	fDefaultsButton->MoveBy(0, -buttonHeight);
	view->AddChild(fDefaultsButton);

	rect = fDefaultsButton->Frame();
	rect.OffsetBy(fDefaultsButton->Bounds().Width() + 10, 0);

	fRevertButton = new BButton(rect, "revert", "Revert",
		new BMessage(kMsgRevert), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fRevertButton->ResizeToPreferred();
	fRevertButton->SetEnabled(false);
	view->AddChild(fRevertButton);

	rect = Bounds();
	rect.top += 5;
	rect.bottom -= 20 + buttonHeight;
	rect.left += 5;
	BTabView *tabView = new BTabView(rect, "tabview", B_WIDTH_FROM_LABEL);

	rect = tabView->ContainerView()->Bounds().InsetByCopy(5, 8);

	fAntialiasingSettings = new AntialiasingSettingsView(rect, "Antialiasing");
	fColorsView = new APRView(rect, "Colors", B_FOLLOW_ALL, B_WILL_DRAW);

	tabView->AddTab(fColorsView);
	tabView->AddTab(fAntialiasingSettings);
		
	view->AddChild(tabView);
	fColorsView->ResizeToPreferred();
	fAntialiasingSettings->ResizeToPreferred();

	fDefaultsButton->SetEnabled(fColorsView->IsDefaultable()
		|| fAntialiasingSettings->IsDefaultable());
	fDefaultsButton->SetTarget(this);
	fRevertButton->SetTarget(this);
}


void
APRWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgUpdate:
			fDefaultsButton->SetEnabled(fColorsView->IsDefaultable()
								|| fAntialiasingSettings->IsDefaultable());
			fRevertButton->SetEnabled(true);
			break;
		case kMsgSetDefaults:
			fColorsView -> MessageReceived(new BMessage(DEFAULT_SETTINGS));
			fAntialiasingSettings->SetDefaults();
			fDefaultsButton->SetEnabled(false);
			fRevertButton->SetEnabled(true);
			break;

		case kMsgRevert:
			fColorsView -> MessageReceived(new BMessage(REVERT_SETTINGS));
			fAntialiasingSettings->Revert();
			fDefaultsButton->SetEnabled(fColorsView->IsDefaultable()
								|| fAntialiasingSettings->IsDefaultable());
			fRevertButton->SetEnabled(false);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
APRWindow::QuitRequested(void)
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(true);
}
