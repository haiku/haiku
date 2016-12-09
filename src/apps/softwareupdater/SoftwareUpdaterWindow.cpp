/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license
 *
 * Authors:
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 */


#include "SoftwareUpdaterWindow.h"

#include <stdio.h>
#include <AppDefs.h>
#include <Application.h>
#include <Button.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <NodeInfo.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <Resources.h>
#include <SeparatorView.h>
#include <String.h>
#include <StringView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SoftwareUpdater"


const uint32 UPDATE_MESSAGE = 'iUPD';
const uint32 EXIT_MESSAGE = 'iEXT';


SoftwareUpdaterWindow::SoftwareUpdaterWindow()
	:
	BWindow(BRect(0, 0, 0, 300), "Software Update",
		B_TITLED_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS | B_NOT_ZOOMABLE),
	fStripeView(NULL)
{
	uint32 updatesAvailable = 45;

	BBitmap* icon = new BBitmap(BRect(0, 0, 31, 31), 0, B_RGBA32);
	team_info teamInfo;
	get_team_info(B_CURRENT_TEAM, &teamInfo);
	app_info appInfo;
	be_roster->GetRunningAppInfo(teamInfo.team, &appInfo);
	BNodeInfo::GetTrackerIcon(&appInfo.ref, icon, B_LARGE_ICON);

	fStripeView = new StripeView(icon);

	BButton* updateButton = new BButton("Update now",
		new BMessage(UPDATE_MESSAGE));

	BButton* exitButton = new BButton("Cancel",
		new BMessage(EXIT_MESSAGE));

	// Form text based on updates available
	BStringView* headerView;
	BStringView* detailView;

	if (updatesAvailable > 0) {
		headerView = new BStringView("header",
			"Software updates are available.", B_WILL_DRAW);
		char detailString[1024];
		snprintf(detailString, 1024, "There are %" B_PRIu32 " updates available"
			" for your system.", updatesAvailable);
		detailView = new BStringView("detail", detailString, B_WILL_DRAW);
		updateButton->MakeDefault(true);
	} else {
		headerView = new BStringView("header",
			"Your system is up to date.", B_WILL_DRAW);
		detailView = new BStringView("detail",
			"No updates are available at this time.",
			B_WILL_DRAW);
		updateButton->Hide();
		exitButton->SetLabel("Quit");
		exitButton->MakeDefault(true);
	}

	BFont font;
	headerView->GetFont(&font);
	font.SetFace(B_BOLD_FACE);
	font.SetSize(font.Size() * 1.5);
	headerView->SetFont(&font, B_FONT_FAMILY_AND_STYLE | B_FONT_SIZE
		| B_FONT_FLAGS);

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 0)
		.Add(fStripeView)
		.AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
			.SetInsets(0, B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			.Add(headerView)
			.Add(detailView)
			.AddStrut(B_USE_DEFAULT_SPACING)
			.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
				.AddGlue()
				.Add(exitButton)
				.Add(updateButton)
			.End()
		.End()
		.AddGlue()
		//.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
	;
	CenterOnScreen();
	Show();
}


SoftwareUpdaterWindow::~SoftwareUpdaterWindow()
{
}


bool
SoftwareUpdaterWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
