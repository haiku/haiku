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
#include <StringView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SoftwareUpdater"


const uint32 UPDATE_MESSAGE = 'iUPD';


SoftwareUpdaterWindow::SoftwareUpdaterWindow()
	:
	BWindow(BRect(0, 0, 500, 300), "Software Update",
		B_TITLED_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS | B_NOT_ZOOMABLE),
	fStripeView(NULL)
{
	BBitmap* icon = new BBitmap(BRect(0, 0, 31, 31), 0, B_RGBA32);

	team_info teamInfo;
	get_team_info(B_CURRENT_TEAM, &teamInfo);
	app_info appInfo;
	be_roster->GetRunningAppInfo(teamInfo.team, &appInfo);
	BNodeInfo::GetTrackerIcon(&appInfo.ref, icon, B_LARGE_ICON);

	fStripeView = new StripeView(icon);
	BStringView* introText = new BStringView("intro",
		"Software updates are available.", B_WILL_DRAW);
	BFont font;
	introText->GetFont(&font);
	font.SetFace(B_BOLD_FACE);
	font.SetSize(font.Size() * 1.5);
	introText->SetFont(&font, B_FONT_FAMILY_AND_STYLE | B_FONT_SIZE
		| B_FONT_FLAGS);

	BButton* updateButton = new BButton("Apply Updates",
		new BMessage(UPDATE_MESSAGE));

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 0)
		.Add(fStripeView)
		.AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
			.SetInsets(0, B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			//.SetInsets(B_USE_WINDOW_SPACING)
			.Add(introText)
			.Add(updateButton)
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
