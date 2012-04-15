/*
 * Copyright 2009-2011 Haiku, Inc.
 * All Rights Reserved. Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 *		Jonas Sundström, jonas@kirilla.com
 */


#include "PreferencesWindow.h"

#include <ctype.h>

#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <FormattingConventions.h>
#include <GroupLayout.h>
#include <Locale.h>
#include <LayoutBuilder.h>
#include <OpenWithTracker.h>
#include <RadioButton.h>
#include <Roster.h>
#include <SeparatorView.h>
#include <Slider.h>
#include <StringView.h>
#include <TextControl.h>
#include <View.h>

#include "BarApp.h"
#include "StatusView.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "PreferencesWindow"

PreferencesWindow::PreferencesWindow(BRect frame)
	:
	BWindow(frame, B_TRANSLATE("Deskbar preferences"), B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS | B_NOT_ZOOMABLE)
{
	// Menu controls
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

	// Applications controls
	fAppsSort = new BCheckBox(B_TRANSLATE("Sort running applications"),
		new BMessage(kSortRunningApps));
	fAppsSortTrackerFirst = new BCheckBox(B_TRANSLATE("Tracker always first"),
		new BMessage(kTrackerFirst));
	fAppsShowExpanders = new BCheckBox(B_TRANSLATE("Show application expander"),
		new BMessage(kSuperExpando));
	fAppsExpandNew = new BCheckBox(B_TRANSLATE("Expand new applications"),
		new BMessage(kExpandNewTeams));
	fAppsHideLabels = new BCheckBox(B_TRANSLATE("Hide application names"),
		new BMessage(kHideLabels));
	fAppsIconSizeSlider = new BSlider("icon_size", B_TRANSLATE("Icon size"),
		NULL, kMinimumIconSize / kIconSizeInterval,
		kMaximumIconSize / kIconSizeInterval, B_HORIZONTAL);
	fAppsIconSizeSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fAppsIconSizeSlider->SetHashMarkCount((kMaximumIconSize - kMinimumIconSize)
		/ kIconSizeInterval + 1);
	fAppsIconSizeSlider->SetLimitLabels(B_TRANSLATE("Small"),
		B_TRANSLATE("Large"));
	fAppsIconSizeSlider->SetModificationMessage(new BMessage(kResizeTeamIcons));

	// Window controls
	fWindowAlwaysOnTop = new BCheckBox(B_TRANSLATE("Always on top"),
		new BMessage(kAlwaysTop));
	fWindowAutoRaise = new BCheckBox(B_TRANSLATE("Auto-raise"),
		new BMessage(kAutoRaise));
	fWindowAutoHide = new BCheckBox(B_TRANSLATE("Auto-hide"),
		new BMessage(kAutoHide));

	// Clock controls
	BMessage* timeInterval12HoursMessage = new BMessage(kTimeIntervalChanged);
	timeInterval12HoursMessage->AddBool("use24HourClock", false);
	fTimeInterval12HourRadioButton = new BRadioButton("time inteval",
		B_TRANSLATE("12 hour"), timeInterval12HoursMessage);

	BMessage* timeInterval24HoursMessage = new BMessage(kTimeIntervalChanged);
	timeInterval24HoursMessage->AddBool("use24HourClock", true);
	fTimeInterval24HourRadioButton = new BRadioButton("time inteval",
		B_TRANSLATE("24 hour"), timeInterval24HoursMessage);

	fShowSeconds = new BCheckBox(B_TRANSLATE("Show seconds"),
		new BMessage(kShowSeconds));
	fShowDayOfWeek = new BCheckBox(B_TRANSLATE("Show day of week"),
		new BMessage(kShowDayOfWeek));
	fShowTimeZone = new BCheckBox(B_TRANSLATE("Show time zone"),
		new BMessage(kShowTimeZone));

	// Get settings from BarApp
	TBarApp* barApp = static_cast<TBarApp*>(be_app);
	desk_settings* settings = barApp->Settings();

	// Menu settings
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

	int32 docCount = settings->recentDocsCount;
	int32 appCount = settings->recentAppsCount;
	int32 folderCount = settings->recentFoldersCount;

	fMenuRecentDocuments->SetValue(settings->recentDocsEnabled);
	fMenuRecentDocumentCount->SetEnabled(settings->recentDocsEnabled);

	fMenuRecentApplications->SetValue(settings->recentAppsEnabled);
	fMenuRecentApplicationCount->SetEnabled(settings->recentAppsEnabled);

	fMenuRecentFolders->SetValue(settings->recentFoldersEnabled);
	fMenuRecentFolderCount->SetEnabled(settings->recentFoldersEnabled);

	BString docString;
	BString appString;
	BString folderString;

	docString << docCount;
	appString << appCount;
	folderString << folderCount;

	fMenuRecentDocumentCount->SetText(docString.String());
	fMenuRecentApplicationCount->SetText(appString.String());
	fMenuRecentFolderCount->SetText(folderString.String());

	// Applications settings
	fAppsSort->SetValue(settings->sortRunningApps);
	fAppsSortTrackerFirst->SetValue(settings->trackerAlwaysFirst);
	fAppsShowExpanders->SetValue(settings->superExpando);
	fAppsExpandNew->SetValue(settings->expandNewTeams);
	fAppsHideLabels->SetValue(settings->hideLabels);
	fAppsIconSizeSlider->SetValue(settings->iconSize / kIconSizeInterval);

	// Window settings
	fWindowAlwaysOnTop->SetValue(settings->alwaysOnTop);
	fWindowAutoRaise->SetValue(settings->autoRaise);
	fWindowAutoHide->SetValue(settings->autoHide);

	// Clock settings
	BFormattingConventions conventions;
	BLocale::Default()->GetFormattingConventions(&conventions);
	if (conventions.Use24HourClock())
		fTimeInterval24HourRadioButton->SetValue(B_CONTROL_ON);
	else
		fTimeInterval12HourRadioButton->SetValue(B_CONTROL_ON);

	TReplicantTray* replicantTray = barApp->BarView()->ReplicantTray();
	if (replicantTray->Time() != NULL) {
		fShowSeconds->SetValue(replicantTray->Time()->ShowSeconds());
		fShowDayOfWeek->SetValue(replicantTray->Time()->ShowDayOfWeek());
		fShowTimeZone->SetValue(replicantTray->Time()->ShowTimeZone());
	} else {
		fShowSeconds->SetValue(settings->showSeconds);
		fShowDayOfWeek->SetValue(settings->showDayOfWeek);
		fShowTimeZone->SetValue(settings->showTimeZone);
	}

	EnableDisableDependentItems();

	// Targets
	fAppsSort->SetTarget(be_app);
	fAppsSortTrackerFirst->SetTarget(be_app);
	fAppsExpandNew->SetTarget(be_app);
	fAppsHideLabels->SetTarget(be_app);
	fAppsIconSizeSlider->SetTarget(be_app);

	fWindowAlwaysOnTop->SetTarget(be_app);
	fWindowAutoRaise->SetTarget(be_app);
	fWindowAutoHide->SetTarget(be_app);

	fTimeInterval12HourRadioButton->SetTarget(replicantTray);
	fTimeInterval24HourRadioButton->SetTarget(replicantTray);

	fShowSeconds->SetTarget(replicantTray);
	fShowDayOfWeek->SetTarget(replicantTray);
	fShowTimeZone->SetTarget(replicantTray);

	// Layout
	fMenuBox = new BBox("fMenuBox");
	fAppsBox = new BBox("fAppsBox");
	fWindowBox = new BBox("fWindowBox");
	fClockBox = new BBox("fClockBox");

	fMenuBox->SetLabel(B_TRANSLATE("Menu"));
	fAppsBox->SetLabel(B_TRANSLATE("Applications"));
	fWindowBox->SetLabel(B_TRANSLATE("Window"));
	fClockBox->SetLabel(B_TRANSLATE("Clock"));

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
			.Add(fAppsHideLabels)
			.AddGroup(B_HORIZONTAL, 0)
				.SetInsets(0, 10, 0, 0)
				.Add(fAppsIconSizeSlider)
				.End()
			.AddGlue()
			.SetInsets(10, 10, 10, 10)
			.End()
		.View();
	fAppsBox->AddChild(view);

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

	BStringView* timeIntervalLabel = new BStringView("interval",
		B_TRANSLATE("Interval"));
	timeIntervalLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED,
		B_SIZE_UNSET));
	timeIntervalLabel->SetLowColor((rgb_color){255, 255, 255, 255});

	BGroupLayout* timeIntervalLayout = new BGroupLayout(B_VERTICAL, 0);
	timeIntervalLayout->SetInsets(20, 0, 0, 0);
	BView* timeIntervalView = new BView("interval", 0, timeIntervalLayout);
	timeIntervalView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	timeIntervalView->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	timeIntervalView->AddChild(fTimeInterval12HourRadioButton);
	timeIntervalView->AddChild(fTimeInterval24HourRadioButton);

	view = BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL, 10)
			.AddGroup(B_VERTICAL, 0)
				.Add(timeIntervalLabel)
				.Add(timeIntervalView)
				.End()
			.AddGroup(B_VERTICAL, 0)
				.Add(fShowSeconds)
				.Add(fShowDayOfWeek)
				.Add(fShowTimeZone)
				.End()
			.Add(new BButton(B_TRANSLATE("Time preferences" B_UTF8_ELLIPSIS),
				new BMessage(kTimePreferences)))
			.AddGlue()
			.SetInsets(10, 10, 10, 10)
			.End()
		.View();
	fClockBox->AddChild(view);

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
	UpdateRecentCounts();
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
			UpdateRecentCounts();
			break;

		case kSuperExpando:
			EnableDisableDependentItems();
			be_app->PostMessage(message);
			break;

		case kStateChanged:
			EnableDisableDependentItems();
			break;

		case kTimePreferences:
			// launch the time prefs app
			be_roster->Launch("application/x-vnd.Haiku-Time");
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
PreferencesWindow::WindowActivated(bool active)
{
	if (!active && IsMinimized())
		PostMessage(B_QUIT_REQUESTED);
}


void
PreferencesWindow::UpdateRecentCounts()
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

	EnableDisableDependentItems();
}


void
PreferencesWindow::EnableDisableDependentItems()
{
	TBarApp* barApp = static_cast<TBarApp*>(be_app);
	if (barApp->BarView()->Vertical()
		&& barApp->BarView()->ExpandoState()) {
		fAppsShowExpanders->SetEnabled(true);
		fAppsExpandNew->SetEnabled(fAppsShowExpanders->Value());
	} else {
		fAppsShowExpanders->SetEnabled(false);
		fAppsExpandNew->SetEnabled(false);
	}

	fMenuRecentDocumentCount->SetEnabled(
		fMenuRecentDocuments->Value() != B_CONTROL_OFF);
	fMenuRecentApplicationCount->SetEnabled(
		fMenuRecentApplications->Value() != B_CONTROL_OFF);
	fMenuRecentFolderCount->SetEnabled(
		fMenuRecentFolders->Value() != B_CONTROL_OFF);

	fWindowAutoRaise->SetEnabled(
		fWindowAlwaysOnTop->Value() == B_CONTROL_OFF);
}
