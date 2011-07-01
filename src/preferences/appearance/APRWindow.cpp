/*
 * Copyright 2002-2011, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm (darkwyrm@earthlink.net)
 *		Alexander von Gluck, kallisti5@unixzen.com
 */


#include "APRWindow.h"

#include <Button.h>
#include <Catalog.h>
#include <GroupLayoutBuilder.h>
#include <Locale.h>
#include <Messenger.h>
#include <SpaceLayoutItem.h>
#include <TabView.h>

#include "APRView.h"
#include "defs.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "APRWindow"


static const uint32 kMsgSetDefaults = 'dflt';
static const uint32 kMsgRevert = 'rvrt';


APRWindow::APRWindow(BRect frame)
	:
	BWindow(frame, B_TRANSLATE_SYSTEM_NAME("Appearance"), B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS, B_ALL_WORKSPACES)
{

	SetLayout(new BGroupLayout(B_HORIZONTAL));

	fDefaultsButton = new BButton("defaults", B_TRANSLATE("Defaults"),
		new BMessage(kMsgSetDefaults), B_WILL_DRAW);

	fRevertButton = new BButton("revert", B_TRANSLATE("Revert"),
		new BMessage(kMsgRevert), B_WILL_DRAW);

	BTabView* tabView = new BTabView("tabview", B_WIDTH_FROM_LABEL);

	fAntialiasingSettings = new AntialiasingSettingsView(
		B_TRANSLATE("Antialiasing"));

	fDecorSettings = new DecorSettingsView(
		B_TRANSLATE("Window Decorator"));

	fColorsView = new APRView(B_TRANSLATE("Colors"), B_WILL_DRAW);

	tabView->AddTab(fColorsView);
	tabView->AddTab(fAntialiasingSettings);
	tabView->AddTab(fDecorSettings);

	fDefaultsButton->SetEnabled(fColorsView->IsDefaultable()
		|| fAntialiasingSettings->IsDefaultable()
		|| fDecorSettings->IsDefaultable());
	fRevertButton->SetEnabled(false);

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(tabView)
		.Add(BSpaceLayoutItem::CreateVerticalStrut(5))
		.Add(BGroupLayoutBuilder(B_HORIZONTAL)
			.Add(fRevertButton)
			.AddGlue()
			.Add(fDefaultsButton)
		)
		.SetInsets(5, 5, 5, 5)
	);
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
