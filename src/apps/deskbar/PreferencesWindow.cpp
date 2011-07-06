/*
 * Copyright 2009-2011 Haiku, Inc.
 * All Rights Reserved. Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonas Sundström, jonas@kirilla.com
 */


#include "PreferencesWindow.h"

#include <Catalog.h>
#include <CheckBox.h>
#include <GroupLayout.h>
#include <Locale.h>
#include <LayoutBuilder.h>
#include <OpenWithTracker.h>
#include <RadioButton.h>
#include <SeparatorView.h>

#include <ctype.h>

#include "BarApp.h"
#include "StatusView.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "PreferencesWindow"

PreferencesWindow::PreferencesWindow(BRect frame)
	:
	BWindow(frame, B_TRANSLATE("Deskbar preferences"), B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS | B_NOT_ZOOMABLE)
{
	// Controls
	fMenuRecentDocuments = new BCheckBox(B_TRANSLATE("Recent documents:"),
		new BMessage(kUpdateRecentCounts));
	fMenuRecentApplications = new BCheckBox(B_TRANSLATE("Recent applications:"),
		new BMessage(kUpdateRecentCounts));
	fMenuRecentFolders = new BCheckBox(B_TRANSLATE("Recent folders:"),
		new BMessage(kUpdateRecentCounts));

	fMenuRecentDocumentCount = new BTextControl(NULL, NULL,
		new BMessage(kUpdateRecentCounts));
	fMenuRecentApplicationCount = new BTextControl(NULL, NULL,
		new BMessage(kUpdateRecentCounts));
	fMenuRecentFolderCount = new BTextControl(NULL, NULL,
		new BMessage(kUpdateRecentCounts));

	fAppsSort = new BCheckBox(B_TRANSLATE("Sort running applications"),
		new BMessage(kSortRunningApps));
	fAppsSortTrackerFirst = new BCheckBox(B_TRANSLATE("Tracker always first"),
		new BMessage(kTrackerFirst));
	fAppsShowExpanders = new BCheckBox(B_TRANSLATE("Show application expander"),
		new BMessage(kSuperExpando));
	fAppsExpandNew = new BCheckBox(B_TRANSLATE("Expand new applications"),
		new BMessage(kExpandNewTeams));

	fClockSeconds = new BCheckBox(B_TRANSLATE("Show seconds"),
		new BMessage(kShowSeconds));
	fWindowAlwaysOnTop = new BCheckBox(B_TRANSLATE("Always on top"),
		new BMessage(kAlwaysTop));
	fWindowAutoRaise = new BCheckBox(B_TRANSLATE("Auto-raise"),
		new BMessage(kAutoRaise));
	fWindowAutoHide = new BCheckBox(B_TRANSLATE("Auto-hide"),
		new BMessage(kAutoHide));

	BTextView* docTextView = fMenuRecentDocumentCount->TextView();
	BTextView* appTextView = fMenuRecentApplicationCount->TextView();
	BTextView* folderTextView = fMenuRecentFolderCount->TextView();

	for (int32 i = 0; i < 256; i++) {
		if (!isdigit(i)) {
			docTextView->DisallowChar(i);
			appTextView->DisallowChar(i);
			folderTextView->DisallowChar(i);
		}
	}

	docTextView->SetMaxBytes(4);
	appTextView->SetMaxBytes(4);
	folderTextView->SetMaxBytes(4);

	// Values
	TBarApp* barApp = static_cast<TBarApp*>(be_app);
	desk_settings* appSettings = barApp->Settings();

	fAppsSort->SetValue(appSettings->sortRunningApps);
	fAppsSortTrackerFirst->SetValue(appSettings->trackerAlwaysFirst);
	fAppsShowExpanders->SetValue(appSettings->superExpando);
	fAppsExpandNew->SetValue(appSettings->expandNewTeams);

	int32 docCount = appSettings->recentDocsCount;
	int32 appCount = appSettings->recentAppsCount;
	int32 folderCount = appSettings->recentFoldersCount;

	fMenuRecentDocuments->SetValue(appSettings->recentDocsEnabled);
	fMenuRecentDocumentCount->SetEnabled(appSettings->recentDocsEnabled);

	fMenuRecentApplications->SetValue(appSettings->recentAppsEnabled);
	fMenuRecentApplicationCount->SetEnabled(appSettings->recentAppsEnabled);

	fMenuRecentFolders->SetValue(appSettings->recentFoldersEnabled);
	fMenuRecentFolderCount->SetEnabled(appSettings->recentFoldersEnabled);

	BString docString;
	BString appString;
	BString folderString;

	docString << docCount;
	appString << appCount;
	folderString << folderCount;

	fMenuRecentDocumentCount->SetText(docString.String());
	fMenuRecentApplicationCount->SetText(appString.String());
	fMenuRecentFolderCount->SetText(folderString.String());

	TReplicantTray* replicantTray = barApp->BarView()->fReplicantTray;

	fClockSeconds->SetValue(replicantTray->ShowingSeconds());

	bool showingClock = barApp->BarView()->ShowingClock();
	fClockSeconds->SetEnabled(showingClock);

	fWindowAlwaysOnTop->SetValue(appSettings->alwaysOnTop);
	fWindowAutoRaise->SetValue(appSettings->autoRaise);
	fWindowAutoHide->SetValue(appSettings->autoHide);

	_EnableDisableDependentItems();

	// Targets
	fAppsSort->SetTarget(be_app);
	fAppsSortTrackerFirst->SetTarget(be_app);
	fAppsExpandNew->SetTarget(be_app);

	fClockSeconds->SetTarget(replicantTray);

	fWindowAlwaysOnTop->SetTarget(be_app);
	fWindowAutoRaise->SetTarget(be_app);
	fWindowAutoHide->SetTarget(be_app);

	// Layout
	fMenuBox = new BBox("fMenuBox");
	fAppsBox = new BBox("fAppsBox");
	fClockBox = new BBox("fClockBox");
	fWindowBox = new BBox("fWindowBox");

	fMenuBox->SetLabel(B_TRANSLATE("Menu"));
	fAppsBox->SetLabel(B_TRANSLATE("Applications"));
	fClockBox->SetLabel(B_TRANSLATE("Clock"));
	fWindowBox->SetLabel(B_TRANSLATE("Window"));

	BView* view;
	view = BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL, 10)
			.AddGroup(B_HORIZONTAL, 0)
				.AddGroup(B_VERTICAL, 0)
					.Add(fMenuRecentDocuments)
					.Add(fMenuRecentFolders)
					.Add(fMenuRecentApplications)
					.End()
				.AddGroup(B_VERTICAL, 0)
					.Add(fMenuRecentDocumentCount)
					.Add(fMenuRecentFolderCount)
					.Add(fMenuRecentApplicationCount)
					.End()
				.End()
			.Add(new BButton(B_TRANSLATE("Edit menu" B_UTF8_ELLIPSIS),
				new BMessage(kEditMenuInTracker)))
			.SetInsets(10, 10, 10, 10)
			.End()
		.View();
	fMenuBox->AddChild(view);

	view = BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL, 1)
			.Add(fAppsSort)
			.Add(fAppsSortTrackerFirst)
			.Add(fAppsShowExpanders)
			.AddGroup(B_HORIZONTAL, 0)
				.SetInsets(20, 0, 0, 0)
				.Add(fAppsExpandNew)
				.End()
			.AddGlue()
			.SetInsets(10, 10, 10, 10)
			.End()
		.View();
	fAppsBox->AddChild(view);

	view = BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL, 1)
			.Add(fClockSeconds)
			.AddGlue()
			.SetInsets(10, 10, 10, 10)
			.End()
		.View();
	fClockBox->AddChild(view);

	view = BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL, 1)
			.Add(fWindowAlwaysOnTop)
			.Add(fWindowAutoRaise)
			.Add(fWindowAutoHide)
			.AddGlue()
			.SetInsets(10, 10, 10, 10)
			.End()
		.View();
	fWindowBox->AddChild(view);

	BLayoutBuilder::Group<>(this)
		.AddGrid(5, 5)
			.Add(fMenuBox, 0, 0)
			.Add(fWindowBox, 1, 0)
			.Add(fAppsBox, 0, 1)
			.Add(fClockBox, 1, 1)
			.SetInsets(10, 10, 10, 10)
			.End()
		.End();

	CenterOnScreen();
}


PreferencesWindow::~PreferencesWindow()
{
	_UpdateRecentCounts();
	be_app->PostMessage(kConfigClose);
}


void
PreferencesWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kEditMenuInTracker:
			OpenWithTracker(B_USER_DESKBAR_DIRECTORY);
			break;

		case kUpdateRecentCounts:
			_UpdateRecentCounts();
			break;

		case kSuperExpando:
			_EnableDisableDependentItems();
			be_app->PostMessage(message);
			break;

		case kStateChanged:
			_EnableDisableDependentItems();
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
PreferencesWindow::_UpdateRecentCounts()
{
	BMessage message(kUpdateRecentCounts);

	int32 docCount = atoi(fMenuRecentDocumentCount->Text());
	int32 appCount = atoi(fMenuRecentApplicationCount->Text());
	int32 folderCount = atoi(fMenuRecentFolderCount->Text());

	message.AddInt32("documents", max_c(0, docCount));
	message.AddInt32("applications", max_c(0, appCount));
	message.AddInt32("folders", max_c(0, folderCount));

	message.AddBool("documentsEnabled", fMenuRecentDocuments->Value());
	message.AddBool("applicationsEnabled", fMenuRecentApplications->Value());
	message.AddBool("foldersEnabled", fMenuRecentFolders->Value());

	be_app->PostMessage(&message);

	_EnableDisableDependentItems();
}


void
PreferencesWindow::_EnableDisableDependentItems()
{
	TBarApp* barApp = static_cast<TBarApp*>(be_app);
	if (barApp->BarView()->Vertical()
		&& barApp->BarView()->Expando()) {
		fAppsShowExpanders->SetEnabled(true);
		fAppsExpandNew->SetEnabled(fAppsShowExpanders->Value());
	} else {
		fAppsShowExpanders->SetEnabled(false);
		fAppsExpandNew->SetEnabled(false);
	}

	if (fMenuRecentDocuments->Value())
		fMenuRecentDocumentCount->SetEnabled(true);
	else
		fMenuRecentDocumentCount->SetEnabled(false);

	if (fMenuRecentApplications->Value())
		fMenuRecentApplicationCount->SetEnabled(true);
	else
		fMenuRecentApplicationCount->SetEnabled(false);

	if (fMenuRecentFolders->Value())
		fMenuRecentFolderCount->SetEnabled(true);
	else
		fMenuRecentFolderCount->SetEnabled(false);

	if (fWindowAlwaysOnTop->Value() == B_CONTROL_ON)
		fWindowAutoRaise->SetEnabled(false);
	else
		fWindowAutoRaise->SetEnabled(true);
}


void
PreferencesWindow::WindowActivated(bool active)
{
	if (!active && IsMinimized())
		PostMessage(B_QUIT_REQUESTED);
}
