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
#include <ControlLook.h>
#include <FormattingConventions.h>
#include <GroupLayout.h>
#include <ListView.h>
#include <Locale.h>
#include <LayoutBuilder.h>
#include <OpenWithTracker.h>
#include <RadioButton.h>
#include <Roster.h>
#include <SeparatorView.h>
#include <Slider.h>
#include <TextControl.h>
#include <View.h>

#include "BarApp.h"
#include "StatusView.h"


static const float kIndentSpacing
	= be_control_look->DefaultItemSpacing() * 2.3;
static const uint32 kSettingsViewChanged = 'Svch';


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PreferencesWindow"


PreferencesWindow::PreferencesWindow(BRect frame)
	:
	BWindow(frame, B_TRANSLATE("Deskbar preferences"), B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS | B_NOT_ZOOMABLE)
{
	// Set the default and intial settings used by default and revert buttons
	_SetDefaultSettings();
	_SetInitialSettings();

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
	fAppsSort = new BCheckBox(B_TRANSLATE("Sort applications by name"),
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
		new BMessage(kResizeTeamIcons), kMinimumIconSize / kIconSizeInterval,
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

	int32 docCount = fInitialSettings.recentDocsCount;
	int32 appCount = fInitialSettings.recentAppsCount;
	int32 folderCount = fInitialSettings.recentFoldersCount;

	fMenuRecentDocuments->SetValue(fInitialSettings.recentDocsEnabled);
	fMenuRecentDocumentCount->SetEnabled(fInitialSettings.recentDocsEnabled);

	fMenuRecentApplications->SetValue(fInitialSettings.recentAppsEnabled);
	fMenuRecentApplicationCount->SetEnabled(fInitialSettings.recentAppsEnabled);

	fMenuRecentFolders->SetValue(fInitialSettings.recentFoldersEnabled);
	fMenuRecentFolderCount->SetEnabled(fInitialSettings.recentFoldersEnabled);

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
	fAppsSort->SetValue(fInitialSettings.sortRunningApps);
	fAppsSortTrackerFirst->SetValue(fInitialSettings.trackerAlwaysFirst);
	fAppsShowExpanders->SetValue(fInitialSettings.superExpando);
	fAppsExpandNew->SetValue(fInitialSettings.expandNewTeams);
	fAppsHideLabels->SetValue(fInitialSettings.hideLabels);
	fAppsIconSizeSlider->SetValue(fInitialSettings.iconSize
		/ kIconSizeInterval);

	// Window settings
	fWindowAlwaysOnTop->SetValue(fInitialSettings.alwaysOnTop);
	fWindowAutoRaise->SetValue(fInitialSettings.autoRaise);
	fWindowAutoHide->SetValue(fInitialSettings.autoHide);

	_EnableDisableDependentItems();

	// Targets
	fAppsSort->SetTarget(be_app);
	fAppsSortTrackerFirst->SetTarget(be_app);
	fAppsShowExpanders->SetTarget(be_app);
	fAppsExpandNew->SetTarget(be_app);
	fAppsHideLabels->SetTarget(be_app);
	fAppsIconSizeSlider->SetTarget(be_app);

	fWindowAlwaysOnTop->SetTarget(be_app);
	fWindowAutoRaise->SetTarget(be_app);
	fWindowAutoHide->SetTarget(be_app);

	// Applications
	BBox* appsSettingsBox = new BBox("applications");
	appsSettingsBox->SetLabel(B_TRANSLATE("Applications"));
	appsSettingsBox->AddChild(BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL, 0)
			.Add(fAppsSort)
			.Add(fAppsSortTrackerFirst)
			.Add(fAppsShowExpanders)
			.AddGroup(B_HORIZONTAL, 0)
				.SetInsets(kIndentSpacing, 0, 0, 0)
				.Add(fAppsExpandNew)
				.End()
			.Add(fAppsHideLabels)
			.AddGroup(B_HORIZONTAL, 0)
				.SetInsets(0, B_USE_DEFAULT_SPACING, 0, 0)
				.Add(fAppsIconSizeSlider)
				.End()
			.AddGlue()
			.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			.End()
		.View());

	// Recent items
	BBox* recentItemsBox = new BBox("recent items");
	recentItemsBox->SetLabel(B_TRANSLATE("Recent items"));
	recentItemsBox->AddChild(BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL, 0)
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
				//.Add(new BButton(B_TRANSLATE("Open in Tracker" B_UTF8_ELLIPSIS),
				//	new BMessage(kOpenInTracker)))
			.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			.End()
		.View());

	// Window
	BBox* windowSettingsBox = new BBox("window");
	windowSettingsBox->SetLabel(B_TRANSLATE("Window"));
	windowSettingsBox->AddChild(BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL, 0)
			.Add(fWindowAlwaysOnTop)
			.Add(fWindowAutoRaise)
			.Add(fWindowAutoHide)
			.AddGlue()
			.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			.End()
		.View());

	// Action Buttons
	fRevertButton = new BButton(B_TRANSLATE("Revert"),
		new BMessage(kRevert));
	fDefaultsButton = new BButton(B_TRANSLATE("Defaults"),
		new BMessage(kDefaults));

	// Layout
	BLayoutBuilder::Group<>(this)
		.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING)
			.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
				.Add(appsSettingsBox)
				.AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
					.Add(recentItemsBox)
					.Add(windowSettingsBox)
				.End()
			.End()
			.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
				.Add(fRevertButton)
				.AddGlue()
				.Add(fDefaultsButton)
				.End()
			.SetInsets(B_USE_DEFAULT_SPACING)
			.End();

	CenterOnScreen();
}


PreferencesWindow::~PreferencesWindow()
{
	_UpdateRecentCounts();
}


void
PreferencesWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kOpenInTracker:
			OpenWithTracker(B_USER_DESKBAR_DIRECTORY);
			break;

		case kUpdatePreferences:
			_EnableDisableDependentItems();
			_UpdateButtons();
			break;

		case kUpdateRecentCounts:
			_UpdateRecentCounts();
			_UpdateButtons();
			break;

		case kStateChanged:
			_EnableDisableDependentItems();
			break;

		case kRevert:
			_UpdatePreferences(fInitialSettings);
			break;

		case kDefaults:
			_UpdatePreferences(fDefaultSettings);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
PreferencesWindow::QuitRequested()
{
	be_app->PostMessage(kConfigQuit);

	return false;
}


void
PreferencesWindow::Show()
{
	if (IsHidden())
		SetWorkspaces(B_CURRENT_WORKSPACE);

	_UpdateButtons();

	BWindow::Show();
}


//	#pragma mark - private methods


void
PreferencesWindow::_EnableDisableDependentItems()
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
	fMenuRecentFolderCount->SetEnabled(
		fMenuRecentFolders->Value() != B_CONTROL_OFF);
	fMenuRecentApplicationCount->SetEnabled(
		fMenuRecentApplications->Value() != B_CONTROL_OFF);

	fWindowAutoRaise->SetEnabled(
		fWindowAlwaysOnTop->Value() == B_CONTROL_OFF);
}


bool
PreferencesWindow::_IsDefaultable()
{
	desk_settings* settings = static_cast<TBarApp*>(be_app)->Settings();

	return fDefaultSettings.sortRunningApps != settings->sortRunningApps
		|| fDefaultSettings.trackerAlwaysFirst != settings->trackerAlwaysFirst
		|| fDefaultSettings.superExpando != settings->superExpando
		|| fDefaultSettings.expandNewTeams != settings->expandNewTeams
		|| fDefaultSettings.hideLabels != settings->hideLabels
		|| fDefaultSettings.iconSize != settings->iconSize
		|| fDefaultSettings.recentAppsEnabled != settings->recentAppsEnabled
		|| fDefaultSettings.recentDocsEnabled != settings->recentDocsEnabled
		|| fDefaultSettings.recentFoldersEnabled
			!= settings->recentFoldersEnabled
		|| fDefaultSettings.recentAppsCount != settings->recentAppsCount
		|| fDefaultSettings.recentDocsCount != settings->recentDocsCount
		|| fDefaultSettings.recentFoldersCount != settings->recentFoldersCount
		|| fDefaultSettings.alwaysOnTop != settings->alwaysOnTop
		|| fDefaultSettings.autoRaise != settings->autoRaise
		|| fDefaultSettings.autoHide != settings->autoHide;
}


bool
PreferencesWindow::_IsRevertable()
{
	desk_settings* settings = static_cast<TBarApp*>(be_app)->Settings();

	return fInitialSettings.sortRunningApps != settings->sortRunningApps
		|| fInitialSettings.trackerAlwaysFirst != settings->trackerAlwaysFirst
		|| fInitialSettings.superExpando != settings->superExpando
		|| fInitialSettings.expandNewTeams != settings->expandNewTeams
		|| fInitialSettings.hideLabels != settings->hideLabels
		|| fInitialSettings.iconSize != settings->iconSize
		|| fInitialSettings.recentAppsEnabled != settings->recentAppsEnabled
		|| fInitialSettings.recentDocsEnabled != settings->recentDocsEnabled
		|| fInitialSettings.recentFoldersEnabled
			!= settings->recentFoldersEnabled
		|| fInitialSettings.recentAppsCount != settings->recentAppsCount
		|| fInitialSettings.recentDocsCount != settings->recentDocsCount
		|| fInitialSettings.recentFoldersCount != settings->recentFoldersCount
		|| fInitialSettings.alwaysOnTop != settings->alwaysOnTop
		|| fInitialSettings.autoRaise != settings->autoRaise
		|| fInitialSettings.autoHide != settings->autoHide;
}


void
PreferencesWindow::_SetDefaultSettings()
{
	// applications
	fDefaultSettings.sortRunningApps = false;
	fDefaultSettings.trackerAlwaysFirst = false;
	fDefaultSettings.superExpando = false;
	fDefaultSettings.expandNewTeams = false;
	fDefaultSettings.hideLabels = false;
	fDefaultSettings.iconSize = kMinimumIconSize;

	// recent items
	fDefaultSettings.recentAppsEnabled = true;
	fDefaultSettings.recentDocsEnabled = true;
	fDefaultSettings.recentFoldersEnabled = true;
	fDefaultSettings.recentAppsCount = 10;
	fDefaultSettings.recentDocsCount = 10;
	fDefaultSettings.recentFoldersCount = 10;

	// window
	fDefaultSettings.alwaysOnTop = false;
	fDefaultSettings.autoRaise = false;
	fDefaultSettings.autoHide = false;
}


void
PreferencesWindow::_SetInitialSettings()
{
	desk_settings* settings = static_cast<TBarApp*>(be_app)->Settings();

	// applications
	fInitialSettings.sortRunningApps = settings->sortRunningApps;
	fInitialSettings.trackerAlwaysFirst = settings->trackerAlwaysFirst;
	fInitialSettings.superExpando = settings->superExpando;
	fInitialSettings.expandNewTeams = settings->expandNewTeams;
	fInitialSettings.hideLabels = settings->hideLabels;
	fInitialSettings.iconSize = settings->iconSize;

	// recent items
	fInitialSettings.recentAppsEnabled = settings->recentAppsEnabled;
	fInitialSettings.recentDocsEnabled = settings->recentDocsEnabled;
	fInitialSettings.recentFoldersEnabled = settings->recentFoldersEnabled;
	fInitialSettings.recentAppsCount = settings->recentAppsCount;
	fInitialSettings.recentDocsCount = settings->recentDocsCount;
	fInitialSettings.recentFoldersCount = settings->recentFoldersCount;

	// window
	fInitialSettings.alwaysOnTop = settings->alwaysOnTop;
	fInitialSettings.autoRaise = settings->autoRaise;
	fInitialSettings.autoHide = settings->autoHide;
}


void
PreferencesWindow::_UpdateButtons()
{
	fRevertButton->SetEnabled(_IsRevertable());
	fDefaultsButton->SetEnabled(_IsDefaultable());
}


void
PreferencesWindow::_UpdatePreferences(pref_settings prefs)
{
	desk_settings* settings = static_cast<TBarApp*>(be_app)->Settings();
	bool updateRecentCounts = false;

	if (settings->sortRunningApps != prefs.sortRunningApps) {
		fAppsSort->SetValue(prefs.sortRunningApps);
		fAppsSort->Invoke();
	}
	if (settings->trackerAlwaysFirst != prefs.trackerAlwaysFirst) {
		fAppsSortTrackerFirst->SetValue(prefs.trackerAlwaysFirst);
		fAppsSortTrackerFirst->Invoke();
	}
	if (settings->superExpando != prefs.superExpando) {
		fAppsShowExpanders->SetValue(prefs.superExpando);
		fAppsShowExpanders->Invoke();
	}
	if (settings->expandNewTeams != prefs.expandNewTeams) {
		fAppsExpandNew->SetValue(prefs.expandNewTeams);
		fAppsExpandNew->Invoke();
	}
	if (settings->hideLabels != prefs.hideLabels) {
		fAppsHideLabels->SetValue(prefs.hideLabels);
		fAppsHideLabels->Invoke();
	}
	if (settings->iconSize != prefs.iconSize) {
		fAppsIconSizeSlider->SetValue(prefs.iconSize / kIconSizeInterval);
		fAppsIconSizeSlider->Invoke();
	}
	if (settings->recentDocsEnabled != prefs.recentDocsEnabled) {
		fMenuRecentDocuments->SetValue(prefs.recentDocsEnabled
			? B_CONTROL_ON : B_CONTROL_OFF);
		updateRecentCounts = true;
	}
	if (settings->recentFoldersEnabled != prefs.recentFoldersEnabled) {
		fMenuRecentFolders->SetValue(prefs.recentFoldersEnabled
			? B_CONTROL_ON : B_CONTROL_OFF);
		updateRecentCounts = true;
	}
	if (settings->recentAppsEnabled != fDefaultSettings.recentAppsEnabled) {
		fMenuRecentApplications->SetValue(prefs.recentAppsEnabled
			? B_CONTROL_ON : B_CONTROL_OFF);
		updateRecentCounts = true;
	}
	if (settings->recentDocsCount != prefs.recentDocsCount) {
		BString docString;
		docString << prefs.recentDocsCount;
		fMenuRecentDocumentCount->SetText(docString.String());
		updateRecentCounts = true;
	}
	if (settings->recentFoldersCount != prefs.recentFoldersCount) {
		BString folderString;
		folderString << prefs.recentFoldersCount;
		fMenuRecentFolderCount->SetText(folderString.String());
		updateRecentCounts = true;
	}
	if (settings->recentAppsCount != prefs.recentAppsCount) {
		BString appString;
		appString << prefs.recentAppsCount;
		fMenuRecentApplicationCount->SetText(appString.String());
		updateRecentCounts = true;
	}
	if (settings->alwaysOnTop != prefs.alwaysOnTop) {
		fWindowAlwaysOnTop->SetValue(prefs.alwaysOnTop);
		fWindowAlwaysOnTop->Invoke();
	}
	if (settings->autoRaise != prefs.autoRaise) {
		fWindowAutoRaise->SetValue(prefs.autoRaise);
		fWindowAutoRaise->Invoke();
	}
	if (settings->autoHide != prefs.autoHide) {
		fWindowAutoHide->SetValue(prefs.autoHide);
		fWindowAutoHide->Invoke();
	}

	if (updateRecentCounts)
		_UpdateRecentCounts();
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
