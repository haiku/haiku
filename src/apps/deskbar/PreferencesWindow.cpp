/*
 * Copyright 2009-2011 Haiku, Inc.
 * All Rights Reserved. Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 *		Jonas Sundström, jonas@kirilla.com
 */


#include "PreferencesWindow.h"

#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <File.h>
#include <FormattingConventions.h>
#include <GroupLayout.h>
#include <ListView.h>
#include <Locale.h>
#include <LayoutBuilder.h>
#include <OpenWithTracker.h>
#include <Path.h>
#include <RadioButton.h>
#include <Roster.h>
#include <SeparatorView.h>
#include <Screen.h>
#include <Slider.h>
#include <SpaceLayoutItem.h>
#include <Spinner.h>
#include <View.h>

#include "BarApp.h"
#include "DeskbarUtils.h"
#include "StatusView.h"


static const char* kSettingsFileName = "prefs_window_settings";


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PreferencesWindow"


PreferencesWindow::PreferencesWindow(BRect frame)
	:
	BWindow(frame, B_TRANSLATE("Deskbar preferences"), B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS | B_NOT_ZOOMABLE)
{
	// Initial settings (used by revert button)
	fSettings = *static_cast<TBarApp*>(be_app)->Settings();

	// Menu controls
	fMenuRecentDocuments = new BCheckBox(B_TRANSLATE("Recent documents:"),
		new BMessage(kUpdateRecentCounts));
	fMenuRecentApplications = new BCheckBox(B_TRANSLATE("Recent applications:"),
		new BMessage(kUpdateRecentCounts));
	fMenuRecentFolders = new BCheckBox(B_TRANSLATE("Recent folders:"),
		new BMessage(kUpdateRecentCounts));

	fMenuRecentDocumentCount = new BSpinner("recent documents", NULL,
		new BMessage(kUpdateRecentCounts));
	fMenuRecentApplicationCount = new BSpinner("recent applications", NULL,
		new BMessage(kUpdateRecentCounts));
	fMenuRecentFolderCount = new BSpinner("recent folders", NULL,
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
	fMenuRecentDocuments->SetValue(fSettings.recentDocsEnabled);
	fMenuRecentDocumentCount->SetEnabled(fSettings.recentDocsEnabled);
	fMenuRecentDocumentCount->SetRange(0, 50);
	fMenuRecentDocumentCount->SetValue(fSettings.recentDocsCount);

	fMenuRecentApplications->SetValue(fSettings.recentAppsEnabled);
	fMenuRecentApplicationCount->SetEnabled(fSettings.recentAppsEnabled);
	fMenuRecentApplicationCount->SetRange(0, 50);
	fMenuRecentApplicationCount->SetValue(fSettings.recentAppsCount);

	fMenuRecentFolders->SetValue(fSettings.recentFoldersEnabled);
	fMenuRecentFolderCount->SetEnabled(fSettings.recentFoldersEnabled);
	fMenuRecentFolderCount->SetRange(0, 50);
	fMenuRecentFolderCount->SetValue(fSettings.recentFoldersCount);

	// Applications settings
	fAppsSort->SetValue(fSettings.sortRunningApps);
	fAppsSortTrackerFirst->SetValue(fSettings.trackerAlwaysFirst);
	fAppsShowExpanders->SetValue(fSettings.superExpando);
	fAppsExpandNew->SetValue(fSettings.expandNewTeams);
	fAppsHideLabels->SetValue(fSettings.hideLabels);
	fAppsIconSizeSlider->SetValue(fSettings.iconSize
		/ kIconSizeInterval);

	// Window settings
	fWindowAlwaysOnTop->SetValue(fSettings.alwaysOnTop);
	fWindowAutoRaise->SetValue(fSettings.autoRaise);
	fWindowAutoHide->SetValue(fSettings.autoHide);

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

	const float spacing = be_control_look->DefaultItemSpacing() * 2.3;

	// Applications
	BBox* appsSettingsBox = new BBox("applications");
	appsSettingsBox->SetLabel(B_TRANSLATE("Applications"));
	appsSettingsBox->AddChild(BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL, 0)
			.Add(fAppsSort)
			.Add(fAppsSortTrackerFirst)
			.Add(fAppsShowExpanders)
			.AddGroup(B_HORIZONTAL, 0)
				.Add(BSpaceLayoutItem::CreateHorizontalStrut(spacing))
				.Add(fAppsExpandNew)
				.End()
			.Add(fAppsHideLabels)
			.AddGlue()
			.Add(BSpaceLayoutItem::CreateVerticalStrut(B_USE_SMALL_SPACING))
			.Add(fAppsIconSizeSlider)
			.SetInsets(B_USE_DEFAULT_SPACING)
			.End()
		.View());

	// Menu
	BBox* menuBox = new BBox("menu");
	menuBox->SetLabel(B_TRANSLATE("Menu"));
	menuBox->AddChild(BLayoutBuilder::Group<>()
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
				.Add(BSpaceLayoutItem::CreateVerticalStrut(
					B_USE_SMALL_SPACING))
				.Add(new BButton(B_TRANSLATE("Edit in Tracker"
					B_UTF8_ELLIPSIS), new BMessage(kEditInTracker)))
			.AddGlue()
			.SetInsets(B_USE_DEFAULT_SPACING)
			.End()
		.View());

	// Window
	BBox* windowSettingsBox = new BBox("window");
	windowSettingsBox->SetLabel(B_TRANSLATE("Window"));
	windowSettingsBox->AddChild(BLayoutBuilder::Group<>()
		.SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET))
		.AddGroup(B_VERTICAL, 0)
			.Add(fWindowAlwaysOnTop)
			.Add(fWindowAutoRaise)
			.Add(fWindowAutoHide)
			.AddGlue()
			.SetInsets(B_USE_DEFAULT_SPACING)
			.End()
		.View());

	// Action Buttons
	fDefaultsButton = new BButton(B_TRANSLATE("Defaults"),
		new BMessage(kDefaults));
	fRevertButton = new BButton(B_TRANSLATE("Revert"),
		new BMessage(kRevert));

	// Layout
	BLayoutBuilder::Group<>(this)
		.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING)
			.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
				.Add(appsSettingsBox)
				.AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
					.Add(menuBox)
					.Add(windowSettingsBox)
				.End()
			.End()
			.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
				.Add(fDefaultsButton)
				.Add(fRevertButton)
				.AddGlue()
				.End()
			.SetInsets(B_USE_WINDOW_SPACING)
			.End();

	BMessage windowSettings;
	BPoint where;
	if (_LoadSettings(&windowSettings) == B_OK
		&& windowSettings.FindPoint("window_position", &where) == B_OK
		&& BScreen(this).Frame().Contains(where)) {
		MoveTo(where);
	} else
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
		case kEditInTracker:
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
			_UpdatePreferences(&fSettings);
			break;

		case kDefaults:
			_UpdatePreferences(
				static_cast<TBarApp*>(be_app)->DefaultSettings());
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
PreferencesWindow::QuitRequested()
{
	BMessage windowSettings;
	windowSettings.AddPoint("window_position", Frame().LeftTop());
	_SaveSettings(&windowSettings);

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
	desk_settings* defaults = static_cast<TBarApp*>(be_app)->DefaultSettings();

	return defaults->sortRunningApps != settings->sortRunningApps
		|| defaults->trackerAlwaysFirst != settings->trackerAlwaysFirst
		|| defaults->superExpando != settings->superExpando
		|| defaults->expandNewTeams != settings->expandNewTeams
		|| defaults->hideLabels != settings->hideLabels
		|| defaults->iconSize != settings->iconSize
		|| defaults->recentAppsEnabled != settings->recentAppsEnabled
		|| defaults->recentDocsEnabled != settings->recentDocsEnabled
		|| defaults->recentFoldersEnabled
			!= settings->recentFoldersEnabled
		|| defaults->recentAppsCount != settings->recentAppsCount
		|| defaults->recentDocsCount != settings->recentDocsCount
		|| defaults->recentFoldersCount != settings->recentFoldersCount
		|| defaults->alwaysOnTop != settings->alwaysOnTop
		|| defaults->autoRaise != settings->autoRaise
		|| defaults->autoHide != settings->autoHide;
}


bool
PreferencesWindow::_IsRevertable()
{
	desk_settings* settings = static_cast<TBarApp*>(be_app)->Settings();

	return fSettings.sortRunningApps != settings->sortRunningApps
		|| fSettings.trackerAlwaysFirst != settings->trackerAlwaysFirst
		|| fSettings.superExpando != settings->superExpando
		|| fSettings.expandNewTeams != settings->expandNewTeams
		|| fSettings.hideLabels != settings->hideLabels
		|| fSettings.iconSize != settings->iconSize
		|| fSettings.recentAppsEnabled != settings->recentAppsEnabled
		|| fSettings.recentDocsEnabled != settings->recentDocsEnabled
		|| fSettings.recentFoldersEnabled
			!= settings->recentFoldersEnabled
		|| fSettings.recentAppsCount != settings->recentAppsCount
		|| fSettings.recentDocsCount != settings->recentDocsCount
		|| fSettings.recentFoldersCount != settings->recentFoldersCount
		|| fSettings.alwaysOnTop != settings->alwaysOnTop
		|| fSettings.autoRaise != settings->autoRaise
		|| fSettings.autoHide != settings->autoHide;
}


status_t
PreferencesWindow::_InitSettingsFile(BFile* file, bool write)
{
	BPath prefsPath;
	status_t status = GetDeskbarSettingsDirectory(prefsPath);
	if (status != B_OK)
		return status;

	status = prefsPath.Append(kSettingsFileName);
	if (status != B_OK)
		return status;

	if (write) {
		status = file->SetTo(prefsPath.Path(),
			B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	} else
		status = file->SetTo(prefsPath.Path(), B_READ_ONLY);

	return status;
}


status_t
PreferencesWindow::_LoadSettings(BMessage* settings)
{
	BFile prefsFile;
	status_t status = _InitSettingsFile(&prefsFile, false);
	if (status != B_OK)
		return status;

	return settings->Unflatten(&prefsFile);
}


status_t
PreferencesWindow::_SaveSettings(BMessage* settings)
{
	BFile prefsFile;
	status_t status = _InitSettingsFile(&prefsFile, true);
	if (status != B_OK)
		return status;

	return settings->Flatten(&prefsFile);
}


void
PreferencesWindow::_UpdateButtons()
{
	fDefaultsButton->SetEnabled(_IsDefaultable());
	fRevertButton->SetEnabled(_IsRevertable());
}


void
PreferencesWindow::_UpdatePreferences(desk_settings* settings)
{
	desk_settings* current = static_cast<TBarApp*>(be_app)->Settings();
	bool updateRecentCounts = false;

	if (current->sortRunningApps != settings->sortRunningApps) {
		fAppsSort->SetValue(settings->sortRunningApps);
		fAppsSort->Invoke();
	}
	if (current->trackerAlwaysFirst != settings->trackerAlwaysFirst) {
		fAppsSortTrackerFirst->SetValue(settings->trackerAlwaysFirst);
		fAppsSortTrackerFirst->Invoke();
	}
	if (current->superExpando != settings->superExpando) {
		fAppsShowExpanders->SetValue(settings->superExpando);
		fAppsShowExpanders->Invoke();
	}
	if (current->expandNewTeams != settings->expandNewTeams) {
		fAppsExpandNew->SetValue(settings->expandNewTeams);
		fAppsExpandNew->Invoke();
	}
	if (current->hideLabels != settings->hideLabels) {
		fAppsHideLabels->SetValue(settings->hideLabels);
		fAppsHideLabels->Invoke();
	}
	if (current->iconSize != settings->iconSize) {
		fAppsIconSizeSlider->SetValue(settings->iconSize / kIconSizeInterval);
		fAppsIconSizeSlider->Invoke();
	}
	if (current->recentDocsEnabled != settings->recentDocsEnabled) {
		fMenuRecentDocuments->SetValue(settings->recentDocsEnabled
			? B_CONTROL_ON : B_CONTROL_OFF);
		updateRecentCounts = true;
	}
	if (current->recentFoldersEnabled != settings->recentFoldersEnabled) {
		fMenuRecentFolders->SetValue(settings->recentFoldersEnabled
			? B_CONTROL_ON : B_CONTROL_OFF);
		updateRecentCounts = true;
	}
	if (current->recentAppsEnabled != settings->recentAppsEnabled) {
		fMenuRecentApplications->SetValue(settings->recentAppsEnabled
			? B_CONTROL_ON : B_CONTROL_OFF);
		updateRecentCounts = true;
	}
	if (current->recentDocsCount != settings->recentDocsCount) {
		fMenuRecentDocumentCount->SetValue(settings->recentDocsCount);
		updateRecentCounts = true;
	}
	if (current->recentFoldersCount != settings->recentFoldersCount) {
		fMenuRecentFolderCount->SetValue(settings->recentFoldersCount);
		updateRecentCounts = true;
	}
	if (current->recentAppsCount != settings->recentAppsCount) {
		fMenuRecentApplicationCount->SetValue(settings->recentAppsCount);
		updateRecentCounts = true;
	}
	if (current->alwaysOnTop != settings->alwaysOnTop) {
		fWindowAlwaysOnTop->SetValue(settings->alwaysOnTop);
		fWindowAlwaysOnTop->Invoke();
	}
	if (current->autoRaise != settings->autoRaise) {
		fWindowAutoRaise->SetValue(settings->autoRaise);
		fWindowAutoRaise->Invoke();
	}
	if (current->autoHide != settings->autoHide) {
		fWindowAutoHide->SetValue(settings->autoHide);
		fWindowAutoHide->Invoke();
	}

	if (updateRecentCounts)
		_UpdateRecentCounts();
}


void
PreferencesWindow::_UpdateRecentCounts()
{
	BMessage message(kUpdateRecentCounts);

	message.AddInt32("documents", fMenuRecentDocumentCount->Value());
	message.AddInt32("applications", fMenuRecentApplicationCount->Value());
	message.AddInt32("folders", fMenuRecentFolderCount->Value());

	message.AddBool("documentsEnabled", fMenuRecentDocuments->Value());
	message.AddBool("applicationsEnabled", fMenuRecentApplications->Value());
	message.AddBool("foldersEnabled", fMenuRecentFolders->Value());

	be_app->PostMessage(&message);

	_EnableDisableDependentItems();
}
