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
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <NodeInfo.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <package/Context.h>
#include <package/RefreshRepositoryRequest.h>
#include <package/PackageRoster.h>
#include <Resources.h>
#include <SeparatorView.h>
#include <String.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SoftwareUpdater"


const uint32 kMsgUpdate = 'iUPD';
const uint32 kMsgExit = 'iEXT';


using namespace BPackageKit;


SoftwareUpdaterWindow::SoftwareUpdaterWindow()
	:
	BWindow(BRect(0, 0, 0, 300), "Software Update",
		B_TITLED_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS | B_NOT_ZOOMABLE),
//	fUpdateManager(NULL),
	fStripeView(NULL),
	fHeaderView(NULL),
	fDetailView(NULL),
	fUpdateButton(NULL),
	fCancelButton(NULL)
{
	fUpdateManager = new UpdateManager(
		BPackageKit::B_PACKAGE_INSTALLATION_LOCATION_HOME);
	fUpdateManager->SetDebugLevel(10);
	fUpdateManager->Init(BPackageManager::B_ADD_INSTALLED_REPOSITORIES
		| BPackageManager::B_ADD_REMOTE_REPOSITORIES);

	BBitmap* icon = new BBitmap(BRect(0, 0, 31, 31), 0, B_RGBA32);
	team_info teamInfo;
	get_team_info(B_CURRENT_TEAM, &teamInfo);
	app_info appInfo;
	be_roster->GetRunningAppInfo(teamInfo.team, &appInfo);
	BNodeInfo::GetTrackerIcon(&appInfo.ref, icon, B_LARGE_ICON);

	fStripeView = new StripeView(icon);

	BButton* fUpdateButton = new BButton("Update now",
		new BMessage(kMsgUpdate));
	fUpdateButton->Hide();

	BButton* fCancelButton = new BButton("Cancel",
		new BMessage(kMsgExit));

	fHeaderView = new BStringView("header",
		"Checking for updates...", B_WILL_DRAW);
	fDetailView = new BStringView("detail", "Contacting software repositories"
		" to check for package updates.", B_WILL_DRAW);

	BFont font;
	fHeaderView->GetFont(&font);
	font.SetFace(B_BOLD_FACE);
	font.SetSize(font.Size() * 1.5);
	fHeaderView->SetFont(&font, B_FONT_FAMILY_AND_STYLE | B_FONT_SIZE
		| B_FONT_FLAGS);

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 0)
		.Add(fStripeView)
		.AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
			.SetInsets(0, B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			.Add(fHeaderView)
			.Add(fDetailView)
			.AddStrut(B_USE_DEFAULT_SPACING)
			.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
				.AddGlue()
				.Add(fCancelButton)
				.Add(fUpdateButton)
			.End()
		.End()
		.AddGlue()
		//.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
	;
	CenterOnScreen();
	Show();

	// Refresh our repos and update UI
	_Refresh();
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


void
SoftwareUpdaterWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgExit:
			QuitRequested();
			break;
		case kMsgUpdate:
			break;
		default:
			BWindow::MessageReceived(message);
	}
}


void
SoftwareUpdaterWindow::_Error(const char* error)
{
	fHeaderView->SetText("Error encountered!");
	fDetailView->SetText(error);
	fUpdateButton->Hide();
	fCancelButton->Show();
}

void
SoftwareUpdaterWindow::_Refresh()
{
	BPackageRoster roster;
	BStringList repoNames(20);

	status_t result = roster.GetRepositoryNames(repoNames);
	if (result != B_OK) {
		_Error("Unable to obtain repository names.");
		return;
	}

	for (int i = 0; i < repoNames.CountStrings(); i++) {
		const BString& repoName = repoNames.StringAt(i);
		BRepositoryConfig repoConfig;
		result = roster.GetRepositoryConfig(repoName, &repoConfig);
		if (result != B_OK) {
			printf("Skipping '%s' repo due to unknown config\n",
				repoName.String());
			continue;
		}
		/*
		BRefreshRepositoryRequest request(context, repoConfig);
		result = request.Process();
		if (result != B_OK) {
			printf("Skipping %s repo due to unreachable repo");
			continue;
		}
		*/
	}
}
