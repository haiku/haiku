/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/


#include "SettingsViews.h"

#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ColorControl.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MenuField.h>
#include <NodeMonitor.h>
#include <Point.h>
#include <RadioButton.h>
#include <StringView.h>

#include "Commands.h"
#include "DeskWindow.h"
#include "Model.h"
#include "Tracker.h"
#include "WidgetAttributeText.h"


static const uint32 kSpaceBarSwitchColor = 'SBsc';

//TODO: defaults should be set in one place only (TrackerSettings.cpp) while
//		being accessible from here.
//		What about adding DefaultValue(), IsDefault() etc... methods to
//		xxxValueSetting ?
static const uint8 kSpaceBarAlpha = 192;
static const rgb_color kDefaultUsedSpaceColor = { 0, 203, 0, kSpaceBarAlpha };
static const rgb_color kDefaultFreeSpaceColor
	= { 255, 255, 255, kSpaceBarAlpha };
static const rgb_color kDefaultWarningSpaceColor
	= { 203, 0, 0, kSpaceBarAlpha };


static void
send_bool_notices(uint32 what, const char* name, bool value)
{
	TTracker* tracker = dynamic_cast<TTracker*>(be_app);
	if (tracker == NULL)
		return;

	BMessage message;
	message.AddBool(name, value);
	tracker->SendNotices(what, &message);
}


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SettingsView"


//	#pragma mark - SettingsView


SettingsView::SettingsView(const char* name)
	:
	BGroupView(name)
{
}


SettingsView::~SettingsView()
{
}


/*!
	The inherited functions should set the default values
	and update the UI gadgets. The latter can by done by
	calling ShowCurrentSettings().
*/
void
SettingsView::SetDefaults()
{
}


/*!
	This function is used by the window to tell whether
	it can ghost the defaults button or not. It doesn't
	shows the default settings, this function should
	return true.
*/
bool
SettingsView::IsDefaultable() const
{
	return true;
}


/*!
	The inherited functions should set the values that was
	active when the settings window opened. It should also
	update the UI widgets accordingly, preferrable by calling
	ShowCurrentSettings().
*/
void
SettingsView::Revert()
{
}


/*!
	This function is called when the window is shown to let
	the settings views record the state to revert to.
*/
void
SettingsView::RecordRevertSettings()
{
}


/*!
	This function is used by the window to tell the view
	to display the current settings in the tracker.
*/
void
SettingsView::ShowCurrentSettings()
{
}


/*!
	This function is used by the window to tell whether
	it can ghost the revert button or not. It it shows the
	reverted settings, this function should return true.
*/
bool
SettingsView::IsRevertable() const
{
	return true;
}


// #pragma mark - DesktopSettingsView


DesktopSettingsView::DesktopSettingsView()
	:
	SettingsView("DesktopSettingsView"),
	fShowDisksIconRadioButton(NULL),
	fMountVolumesOntoDesktopRadioButton(NULL),
	fMountSharedVolumesOntoDesktopCheckBox(NULL),
	fShowDisksIcon(false),
	fMountVolumesOntoDesktop(false),
	fMountSharedVolumesOntoDesktop(false),
	fIntegrateNonBootBeOSDesktops(false),
	fEjectWhenUnmounting(false)
{
	fShowDisksIconRadioButton = new BRadioButton("",
		B_TRANSLATE("Show Disks icon"),
		new BMessage(kShowDisksIconChanged));

	fMountVolumesOntoDesktopRadioButton = new BRadioButton("",
		B_TRANSLATE("Show volumes on Desktop"),
		new BMessage(kVolumesOnDesktopChanged));

	fMountSharedVolumesOntoDesktopCheckBox = new BCheckBox("",
		B_TRANSLATE("Show shared volumes on Desktop"),
		new BMessage(kVolumesOnDesktopChanged));

	const float spacing = be_control_look->DefaultItemSpacing();

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(fShowDisksIconRadioButton)
		.Add(fMountVolumesOntoDesktopRadioButton)
		.AddGroup(B_VERTICAL, 0)
			.Add(fMountSharedVolumesOntoDesktopCheckBox)
			.SetInsets(spacing * 2, 0, 0, 0)
			.End()
		.AddGlue()
		.SetInsets(spacing);
}


void
DesktopSettingsView::AttachedToWindow()
{
	fShowDisksIconRadioButton->SetTarget(this);
	fMountVolumesOntoDesktopRadioButton->SetTarget(this);
	fMountSharedVolumesOntoDesktopCheckBox->SetTarget(this);
}


void
DesktopSettingsView::MessageReceived(BMessage* message)
{
	TTracker* tracker = dynamic_cast<TTracker*>(be_app);
	if (tracker == NULL)
		return;

	TrackerSettings settings;

	switch (message->what) {
		case kShowDisksIconChanged:
		{
			// Turn on and off related settings:
			fMountVolumesOntoDesktopRadioButton->SetValue(
				!(fShowDisksIconRadioButton->Value() == 1));
			fMountSharedVolumesOntoDesktopCheckBox->SetEnabled(
				fMountVolumesOntoDesktopRadioButton->Value() == 1);

			// Set the new settings in the tracker:
			settings.SetShowDisksIcon(fShowDisksIconRadioButton->Value() == 1);
			settings.SetMountVolumesOntoDesktop(
				fMountVolumesOntoDesktopRadioButton->Value() == 1);
			settings.SetMountSharedVolumesOntoDesktop(
				fMountSharedVolumesOntoDesktopCheckBox->Value() == 1);

			// Construct the notification message:
			BMessage notificationMessage;
			notificationMessage.AddBool("ShowDisksIcon",
				fShowDisksIconRadioButton->Value() == 1);
			notificationMessage.AddBool("MountVolumesOntoDesktop",
				fMountVolumesOntoDesktopRadioButton->Value() == 1);
			notificationMessage.AddBool("MountSharedVolumesOntoDesktop",
				fMountSharedVolumesOntoDesktopCheckBox->Value() == 1);

			// Send the notification message:
			tracker->SendNotices(kVolumesOnDesktopChanged,
				&notificationMessage);

			// Tell the settings window the contents have changed:
			Window()->PostMessage(kSettingsContentsModified);
			break;
		}

		case kVolumesOnDesktopChanged:
		{
			// Turn on and off related settings:
			fShowDisksIconRadioButton->SetValue(
				!(fMountVolumesOntoDesktopRadioButton->Value() == 1));
			fMountSharedVolumesOntoDesktopCheckBox->SetEnabled(
				fMountVolumesOntoDesktopRadioButton->Value() == 1);

			// Set the new settings in the tracker:
			settings.SetShowDisksIcon(fShowDisksIconRadioButton->Value() == 1);
			settings.SetMountVolumesOntoDesktop(
				fMountVolumesOntoDesktopRadioButton->Value() == 1);
			settings.SetMountSharedVolumesOntoDesktop(
				fMountSharedVolumesOntoDesktopCheckBox->Value() == 1);

			// Construct the notification message:
			BMessage notificationMessage;
			notificationMessage.AddBool("ShowDisksIcon",
				fShowDisksIconRadioButton->Value() == 1);
			notificationMessage.AddBool("MountVolumesOntoDesktop",
				fMountVolumesOntoDesktopRadioButton->Value() == 1);
			notificationMessage.AddBool("MountSharedVolumesOntoDesktop",
				fMountSharedVolumesOntoDesktopCheckBox->Value() == 1);

			// Send the notification message:
			tracker->SendNotices(kVolumesOnDesktopChanged,\
				&notificationMessage);

			// Tell the settings window the contents have changed:
			Window()->PostMessage(kSettingsContentsModified);
			break;
		}

		default:
			_inherited::MessageReceived(message);
			break;
	}
}


void
DesktopSettingsView::SetDefaults()
{
	// ToDo: Avoid the duplication of the default values.
	TrackerSettings settings;

	settings.SetShowDisksIcon(false);
	settings.SetMountVolumesOntoDesktop(true);
	settings.SetMountSharedVolumesOntoDesktop(true);
	settings.SetEjectWhenUnmounting(true);

	ShowCurrentSettings();
	_SendNotices();
}


bool
DesktopSettingsView::IsDefaultable() const
{
	TrackerSettings settings;

	return settings.ShowDisksIcon() != false
		|| settings.MountVolumesOntoDesktop() != true
		|| settings.MountSharedVolumesOntoDesktop() != true
		|| settings.EjectWhenUnmounting() != true;
}


void
DesktopSettingsView::Revert()
{
	TrackerSettings settings;

	settings.SetShowDisksIcon(fShowDisksIcon);
	settings.SetMountVolumesOntoDesktop(fMountVolumesOntoDesktop);
	settings.SetMountSharedVolumesOntoDesktop(fMountSharedVolumesOntoDesktop);
	settings.SetEjectWhenUnmounting(fEjectWhenUnmounting);

	ShowCurrentSettings();
	_SendNotices();
}


void
DesktopSettingsView::_SendNotices()
{
	TTracker* tracker = dynamic_cast<TTracker*>(be_app);
	if (tracker == NULL)
		return;

	// Construct the notification message:
	BMessage notificationMessage;
	notificationMessage.AddBool("ShowDisksIcon",
		fShowDisksIconRadioButton->Value() == 1);
	notificationMessage.AddBool("MountVolumesOntoDesktop",
		fMountVolumesOntoDesktopRadioButton->Value() == 1);
	notificationMessage.AddBool("MountSharedVolumesOntoDesktop",
		fMountSharedVolumesOntoDesktopCheckBox->Value() == 1);

	// Send notices to the tracker about the change:
	tracker->SendNotices(kVolumesOnDesktopChanged, &notificationMessage);
	tracker->SendNotices(kDesktopIntegrationChanged, &notificationMessage);
}


void
DesktopSettingsView::ShowCurrentSettings()
{
	TrackerSettings settings;

	fShowDisksIconRadioButton->SetValue(settings.ShowDisksIcon());
	fMountVolumesOntoDesktopRadioButton->SetValue(
		settings.MountVolumesOntoDesktop());

	fMountSharedVolumesOntoDesktopCheckBox->SetValue(
		settings.MountSharedVolumesOntoDesktop());
	fMountSharedVolumesOntoDesktopCheckBox->SetEnabled(
		settings.MountVolumesOntoDesktop());
}


void
DesktopSettingsView::RecordRevertSettings()
{
	TrackerSettings settings;

	fShowDisksIcon = settings.ShowDisksIcon();
	fMountVolumesOntoDesktop = settings.MountVolumesOntoDesktop();
	fMountSharedVolumesOntoDesktop = settings.MountSharedVolumesOntoDesktop();
	fEjectWhenUnmounting = settings.EjectWhenUnmounting();
}


bool
DesktopSettingsView::IsRevertable() const
{
	return fShowDisksIcon != (fShowDisksIconRadioButton->Value() > 0)
		|| fMountVolumesOntoDesktop !=
			(fMountVolumesOntoDesktopRadioButton->Value() > 0)
		|| fMountSharedVolumesOntoDesktop !=
			(fMountSharedVolumesOntoDesktopCheckBox->Value() > 0);
}


// #pragma mark - WindowsSettingsView


WindowsSettingsView::WindowsSettingsView()
	:
	SettingsView("WindowsSettingsView"),
	fShowFullPathInTitleBarCheckBox(NULL),
	fSingleWindowBrowseCheckBox(NULL),
	fShowNavigatorCheckBox(NULL),
	fOutlineSelectionCheckBox(NULL),
	fSortFolderNamesFirstCheckBox(NULL),
	fHideDotFilesCheckBox(NULL),
	fTypeAheadFilteringCheckBox(NULL),
	fGenerateImageThumbnailsCheckBox(NULL),
	fShowFullPathInTitleBar(false),
	fSingleWindowBrowse(false),
	fShowNavigator(false),
	fTransparentSelection(false),
	fSortFolderNamesFirst(false),
	fHideDotFiles(false),
	fTypeAheadFiltering(false)
{
	fShowFullPathInTitleBarCheckBox = new BCheckBox("",
		B_TRANSLATE("Show folder location in title tab"),
		new BMessage(kWindowsShowFullPathChanged));

	fSingleWindowBrowseCheckBox = new BCheckBox("",
		B_TRANSLATE("Single window navigation"),
		new BMessage(kSingleWindowBrowseChanged));

	fShowNavigatorCheckBox = new BCheckBox("",
		B_TRANSLATE("Show navigator"),
		new BMessage(kShowNavigatorChanged));

	fOutlineSelectionCheckBox = new BCheckBox("",
		B_TRANSLATE("Outline selection rectangle only"),
		new BMessage(kTransparentSelectionChanged));

	fSortFolderNamesFirstCheckBox = new BCheckBox("",
		B_TRANSLATE("List folders first"),
		new BMessage(kSortFolderNamesFirstChanged));

	fHideDotFilesCheckBox = new BCheckBox("",
		B_TRANSLATE("Hide dotfiles"),
		new BMessage(kHideDotFilesChanged));

	fTypeAheadFilteringCheckBox = new BCheckBox("",
		B_TRANSLATE("Enable type-ahead filtering"),
		new BMessage(kTypeAheadFilteringChanged));

	fGenerateImageThumbnailsCheckBox = new BCheckBox("",
		B_TRANSLATE("Generate image thumbnails"),
		new BMessage(kGenerateImageThumbnailsChanged));

	const float spacing = be_control_look->DefaultItemSpacing();

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.AddGroup(B_VERTICAL, 0)
			.Add(fShowFullPathInTitleBarCheckBox)
			.Add(fSingleWindowBrowseCheckBox)
			.End()
		.AddGroup(B_VERTICAL)
			.Add(fShowNavigatorCheckBox)
			.SetInsets(spacing * 2, 0, 0, 0)
			.End()
		.AddGroup(B_VERTICAL, 0)
			.Add(fOutlineSelectionCheckBox)
			.Add(fSortFolderNamesFirstCheckBox)
			.Add(fHideDotFilesCheckBox)
			.Add(fTypeAheadFilteringCheckBox)
			.Add(fGenerateImageThumbnailsCheckBox)
			.End()
		.AddGlue()
		.SetInsets(spacing);
}


void
WindowsSettingsView::AttachedToWindow()
{
	fSingleWindowBrowseCheckBox->SetTarget(this);
	fShowNavigatorCheckBox->SetTarget(this);
	fShowFullPathInTitleBarCheckBox->SetTarget(this);
	fOutlineSelectionCheckBox->SetTarget(this);
	fSortFolderNamesFirstCheckBox->SetTarget(this);
	fHideDotFilesCheckBox->SetTarget(this);
	fTypeAheadFilteringCheckBox->SetTarget(this);
	fGenerateImageThumbnailsCheckBox->SetTarget(this);
}


void
WindowsSettingsView::MessageReceived(BMessage* message)
{
	TTracker* tracker = dynamic_cast<TTracker*>(be_app);
	if (tracker == NULL)
		return;

	TrackerSettings settings;

	switch (message->what) {
		case kWindowsShowFullPathChanged:
			settings.SetShowFullPathInTitleBar(
				fShowFullPathInTitleBarCheckBox->Value() == 1);
			tracker->SendNotices(kWindowsShowFullPathChanged);
			Window()->PostMessage(kSettingsContentsModified);
			break;

		case kSingleWindowBrowseChanged:
			settings.SetSingleWindowBrowse(
				fSingleWindowBrowseCheckBox->Value() == 1);
			if (fSingleWindowBrowseCheckBox->Value() == 0) {
				fShowNavigatorCheckBox->SetEnabled(false);
				settings.SetShowNavigator(0);
			} else {
				fShowNavigatorCheckBox->SetEnabled(true);
				settings.SetShowNavigator(
					fShowNavigatorCheckBox->Value() != 0);
			}
			tracker->SendNotices(kShowNavigatorChanged);
			tracker->SendNotices(kSingleWindowBrowseChanged);
			Window()->PostMessage(kSettingsContentsModified);
			break;

		case kShowNavigatorChanged:
			settings.SetShowNavigator(fShowNavigatorCheckBox->Value() == 1);
			tracker->SendNotices(kShowNavigatorChanged);
			Window()->PostMessage(kSettingsContentsModified);
			break;

		case kTransparentSelectionChanged:
		{
			settings.SetTransparentSelection(
				fOutlineSelectionCheckBox->Value() == B_CONTROL_OFF);

			// Make the notification message and send it to the tracker:
			send_bool_notices(kTransparentSelectionChanged,
				"TransparentSelection", settings.TransparentSelection());

			Window()->PostMessage(kSettingsContentsModified);
			break;
		}

		case kSortFolderNamesFirstChanged:
		{
			settings.SetSortFolderNamesFirst(
				fSortFolderNamesFirstCheckBox->Value() == 1);

			// Make the notification message and send it to the tracker:
			send_bool_notices(kSortFolderNamesFirstChanged,
				"SortFolderNamesFirst",
				fSortFolderNamesFirstCheckBox->Value() == 1);

			Window()->PostMessage(kSettingsContentsModified);
			break;
		}

		case kHideDotFilesChanged:
		{
			settings.SetHideDotFiles(
				fHideDotFilesCheckBox->Value() == 1);

			// Make the notification message and send it to the tracker:
			send_bool_notices(kHideDotFilesChanged,
				"HideDotFiles",
				fHideDotFilesCheckBox->Value() == 1);

			Window()->PostMessage(kSettingsContentsModified);
			break;
		}

		case kTypeAheadFilteringChanged:
		{
			settings.SetTypeAheadFiltering(
				fTypeAheadFilteringCheckBox->Value() == 1);
			send_bool_notices(kTypeAheadFilteringChanged,
				"TypeAheadFiltering",
				fTypeAheadFilteringCheckBox->Value() == 1);
			Window()->PostMessage(kSettingsContentsModified);
			break;
		}

		case kGenerateImageThumbnailsChanged:
		{
			settings.SetGenerateImageThumbnails(
				fGenerateImageThumbnailsCheckBox->Value() == 1);
			send_bool_notices(kGenerateImageThumbnailsChanged,
				"GenerateImageThumbnails",
				fGenerateImageThumbnailsCheckBox->Value() == 1);
			Window()->PostMessage(kSettingsContentsModified);
			break;
		}

		default:
			_inherited::MessageReceived(message);
			break;
	}
}


void
WindowsSettingsView::SetDefaults()
{
	TTracker* tracker = dynamic_cast<TTracker*>(be_app);
	if (tracker == NULL)
		return;

	TrackerSettings settings;

	if (settings.ShowFullPathInTitleBar()) {
		settings.SetShowFullPathInTitleBar(false);
		tracker->SendNotices(kWindowsShowFullPathChanged);
	}

	if (settings.SingleWindowBrowse()) {
		settings.SetSingleWindowBrowse(false);
		tracker->SendNotices(kSingleWindowBrowseChanged);
	}

	if (settings.ShowNavigator()) {
		settings.SetShowNavigator(false);
		tracker->SendNotices(kShowNavigatorChanged);
	}

	if (!settings.TransparentSelection()) {
		settings.SetTransparentSelection(true);
		send_bool_notices(kTransparentSelectionChanged,
			"TransparentSelection", true);
	}

	if (!settings.SortFolderNamesFirst()) {
		settings.SetSortFolderNamesFirst(true);
		send_bool_notices(kSortFolderNamesFirstChanged,
			"SortFolderNamesFirst", true);
	}

	if (!settings.HideDotFiles()) {
		settings.SetHideDotFiles(true);
		send_bool_notices(kHideDotFilesChanged,
			"HideDotFiles", true);
	}

	if (settings.TypeAheadFiltering()) {
		settings.SetTypeAheadFiltering(false);
		send_bool_notices(kTypeAheadFilteringChanged,
			"TypeAheadFiltering", true);
	}

	if (settings.GenerateImageThumbnails()) {
		settings.SetGenerateImageThumbnails(false);
		send_bool_notices(kGenerateImageThumbnailsChanged,
			"GenerateImageThumbnails", true);
	}

	ShowCurrentSettings();
}


bool
WindowsSettingsView::IsDefaultable() const
{
	TrackerSettings settings;

	return settings.ShowFullPathInTitleBar() != false
		|| settings.SingleWindowBrowse() != false
		|| settings.ShowNavigator() != false
		|| settings.TransparentSelection() != true
		|| settings.SortFolderNamesFirst() != true
		|| settings.TypeAheadFiltering() != false
		|| settings.GenerateImageThumbnails() != false;
}


void
WindowsSettingsView::Revert()
{
	TTracker* tracker = dynamic_cast<TTracker*>(be_app);
	if (tracker == NULL)
		return;

	TrackerSettings settings;

	if (settings.ShowFullPathInTitleBar() != fShowFullPathInTitleBar) {
		settings.SetShowFullPathInTitleBar(fShowFullPathInTitleBar);
		tracker->SendNotices(kWindowsShowFullPathChanged);
	}

	if (settings.SingleWindowBrowse() != fSingleWindowBrowse) {
		settings.SetSingleWindowBrowse(fSingleWindowBrowse);
		tracker->SendNotices(kSingleWindowBrowseChanged);
	}

	if (settings.ShowNavigator() != fShowNavigator) {
		settings.SetShowNavigator(fShowNavigator);
		tracker->SendNotices(kShowNavigatorChanged);
	}

	if (settings.TransparentSelection() != fTransparentSelection) {
		settings.SetTransparentSelection(fTransparentSelection);
		send_bool_notices(kTransparentSelectionChanged,
			"TransparentSelection", fTransparentSelection);
	}

	if (settings.SortFolderNamesFirst() != fSortFolderNamesFirst) {
		settings.SetSortFolderNamesFirst(fSortFolderNamesFirst);
		send_bool_notices(kSortFolderNamesFirstChanged,
			"SortFolderNamesFirst", fSortFolderNamesFirst);
	}

	if (settings.HideDotFiles() != fHideDotFiles) {
		settings.SetSortFolderNamesFirst(fHideDotFiles);
		send_bool_notices(kHideDotFilesChanged,
			"HideDotFiles", fHideDotFiles);
	}

	if (settings.TypeAheadFiltering() != fTypeAheadFiltering) {
		settings.SetTypeAheadFiltering(fTypeAheadFiltering);
		send_bool_notices(kTypeAheadFilteringChanged,
			"TypeAheadFiltering", fTypeAheadFiltering);
	}

	if (settings.GenerateImageThumbnails() != fGenerateImageThumbnails) {
		settings.SetGenerateImageThumbnails(fGenerateImageThumbnails);
		send_bool_notices(kGenerateImageThumbnailsChanged,
			"GenerateImageThumbnails", fGenerateImageThumbnails);
	}

	ShowCurrentSettings();
}


void
WindowsSettingsView::ShowCurrentSettings()
{
	TrackerSettings settings;

	fShowFullPathInTitleBarCheckBox->SetValue(
		settings.ShowFullPathInTitleBar());
	fSingleWindowBrowseCheckBox->SetValue(settings.SingleWindowBrowse());
	fShowNavigatorCheckBox->SetEnabled(settings.SingleWindowBrowse());
	fShowNavigatorCheckBox->SetValue(settings.ShowNavigator());
	fOutlineSelectionCheckBox->SetValue(settings.TransparentSelection()
		? B_CONTROL_OFF : B_CONTROL_ON);
	fSortFolderNamesFirstCheckBox->SetValue(settings.SortFolderNamesFirst());
	fHideDotFilesCheckBox->SetValue(settings.HideDotFiles());
	fTypeAheadFilteringCheckBox->SetValue(settings.TypeAheadFiltering());
	fGenerateImageThumbnailsCheckBox->SetValue(
		settings.GenerateImageThumbnails());
}


void
WindowsSettingsView::RecordRevertSettings()
{
	TrackerSettings settings;

	fShowFullPathInTitleBar = settings.ShowFullPathInTitleBar();
	fSingleWindowBrowse = settings.SingleWindowBrowse();
	fShowNavigator = settings.ShowNavigator();
	fTransparentSelection = settings.TransparentSelection();
	fSortFolderNamesFirst = settings.SortFolderNamesFirst();
	fHideDotFiles = settings.HideDotFiles();
	fTypeAheadFiltering = settings.TypeAheadFiltering();
	fGenerateImageThumbnails = settings.GenerateImageThumbnails();
}


bool
WindowsSettingsView::IsRevertable() const
{
	TrackerSettings settings;

	return fShowFullPathInTitleBar != settings.ShowFullPathInTitleBar()
		|| fSingleWindowBrowse != settings.SingleWindowBrowse()
		|| fShowNavigator != settings.ShowNavigator()
		|| fTransparentSelection != settings.TransparentSelection()
		|| fSortFolderNamesFirst != settings.SortFolderNamesFirst()
		|| fHideDotFiles != settings.HideDotFiles()
		|| fTypeAheadFiltering != settings.TypeAheadFiltering()
		|| fGenerateImageThumbnails != settings.GenerateImageThumbnails();
}


// #pragma mark - SpaceBarSettingsView


SpaceBarSettingsView::SpaceBarSettingsView()
	:
	SettingsView("SpaceBarSettingsView")
{
	fSpaceBarShowCheckBox = new BCheckBox("",
		B_TRANSLATE("Show space bars on volumes"),
		new BMessage(kUpdateVolumeSpaceBar));

	BPopUpMenu* menu = new BPopUpMenu(B_EMPTY_STRING);
	menu->SetFont(be_plain_font);

	BMenuItem* item;
	menu->AddItem(item = new BMenuItem(
		B_TRANSLATE("Used space color"),
		new BMessage(kSpaceBarSwitchColor)));
	item->SetMarked(true);
	fCurrentColor = 0;
	menu->AddItem(new BMenuItem(
		B_TRANSLATE("Free space color"),
		new BMessage(kSpaceBarSwitchColor)));
	menu->AddItem(new BMenuItem(
		B_TRANSLATE("Warning space color"),
		new BMessage(kSpaceBarSwitchColor)));

	fColorPicker = new BMenuField("menu", NULL, menu);

	fColorControl = new BColorControl(BPoint(0, 0),
		B_CELLS_16x16, 1, "SpaceColorControl",
		new BMessage(kSpaceBarColorChanged));
	fColorControl->SetValue(TrackerSettings().UsedSpaceColor());

	BBox* box = new BBox("box");
	box->SetLabel(fColorPicker);
	box->AddChild(BLayoutBuilder::Group<>(B_HORIZONTAL)
		.Add(fColorControl)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.View());

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(fSpaceBarShowCheckBox)
		.Add(box)
		.AddGlue()
		.SetInsets(B_USE_DEFAULT_SPACING);
}


SpaceBarSettingsView::~SpaceBarSettingsView()
{
}


void
SpaceBarSettingsView::AttachedToWindow()
{
	fSpaceBarShowCheckBox->SetTarget(this);
	fColorControl->SetTarget(this);
	fColorPicker->Menu()->SetTargetForItems(this);
}


void
SpaceBarSettingsView::MessageReceived(BMessage* message)
{
	TTracker* tracker = dynamic_cast<TTracker*>(be_app);
	if (tracker == NULL)
		return;

	TrackerSettings settings;

	switch (message->what) {
		case kUpdateVolumeSpaceBar:
		{
			settings.SetShowVolumeSpaceBar(
				fSpaceBarShowCheckBox->Value() == 1);
			Window()->PostMessage(kSettingsContentsModified);
			tracker->PostMessage(kShowVolumeSpaceBar);
			break;
		}

		case kSpaceBarSwitchColor:
		{
			fCurrentColor = message->FindInt32("index");
			switch (fCurrentColor) {
				case 0:
					fColorControl->SetValue(settings.UsedSpaceColor());
					break;

				case 1:
					fColorControl->SetValue(settings.FreeSpaceColor());
					break;

				case 2:
					fColorControl->SetValue(settings.WarningSpaceColor());
					break;
			}
			break;
		}

		case kSpaceBarColorChanged:
		{
			rgb_color color = fColorControl->ValueAsColor();
			color.alpha = kSpaceBarAlpha;
				// alpha is ignored by BColorControl but is checked
				// in equalities

			switch (fCurrentColor) {
				case 0:
					settings.SetUsedSpaceColor(color);
					break;

				case 1:
					settings.SetFreeSpaceColor(color);
					break;

				case 2:
					settings.SetWarningSpaceColor(color);
					break;
			}

			BWindow* window = Window();
			if (window != NULL)
				window->PostMessage(kSettingsContentsModified);

			tracker->PostMessage(kSpaceBarColorChanged);
			break;
		}

		default:
			_inherited::MessageReceived(message);
			break;
	}
}


void
SpaceBarSettingsView::SetDefaults()
{
	TTracker* tracker = dynamic_cast<TTracker*>(be_app);
	if (tracker == NULL)
		return;

	TrackerSettings settings;

	if (!settings.ShowVolumeSpaceBar()) {
		settings.SetShowVolumeSpaceBar(true);
		send_bool_notices(kShowVolumeSpaceBar, "ShowVolumeSpaceBar", true);
	}

	if (settings.UsedSpaceColor() != kDefaultUsedSpaceColor
		|| settings.FreeSpaceColor() != kDefaultFreeSpaceColor
		|| settings.WarningSpaceColor() != kDefaultWarningSpaceColor) {
		settings.SetUsedSpaceColor(kDefaultUsedSpaceColor);
		settings.SetFreeSpaceColor(kDefaultFreeSpaceColor);
		settings.SetWarningSpaceColor(kDefaultWarningSpaceColor);
		tracker->SendNotices(kSpaceBarColorChanged);
	}

	ShowCurrentSettings();
}


bool
SpaceBarSettingsView::IsDefaultable() const
{
	TrackerSettings settings;

	return settings.ShowVolumeSpaceBar() != true
		|| settings.UsedSpaceColor() != kDefaultUsedSpaceColor
		|| settings.FreeSpaceColor() != kDefaultFreeSpaceColor
		|| settings.WarningSpaceColor() != kDefaultWarningSpaceColor;
}


void
SpaceBarSettingsView::Revert()
{
	TTracker* tracker = dynamic_cast<TTracker*>(be_app);
	if (tracker == NULL)
		return;

	TrackerSettings settings;

	if (settings.ShowVolumeSpaceBar() != fSpaceBarShow) {
		settings.SetShowVolumeSpaceBar(fSpaceBarShow);
		send_bool_notices(kShowVolumeSpaceBar, "ShowVolumeSpaceBar",
			fSpaceBarShow);
	}

	if (settings.UsedSpaceColor() != fUsedSpaceColor
		|| settings.FreeSpaceColor() != fFreeSpaceColor
		|| settings.WarningSpaceColor() != fWarningSpaceColor) {
		settings.SetUsedSpaceColor(fUsedSpaceColor);
		settings.SetFreeSpaceColor(fFreeSpaceColor);
		settings.SetWarningSpaceColor(fWarningSpaceColor);
		tracker->SendNotices(kSpaceBarColorChanged);
	}

	ShowCurrentSettings();
}


void
SpaceBarSettingsView::ShowCurrentSettings()
{
	TrackerSettings settings;

	fSpaceBarShowCheckBox->SetValue(settings.ShowVolumeSpaceBar());

	switch (fCurrentColor) {
		case 0:
			fColorControl->SetValue(settings.UsedSpaceColor());
			break;
		case 1:
			fColorControl->SetValue(settings.FreeSpaceColor());
			break;
		case 2:
			fColorControl->SetValue(settings.WarningSpaceColor());
			break;
	}
}


void
SpaceBarSettingsView::RecordRevertSettings()
{
	TrackerSettings settings;

	fSpaceBarShow = settings.ShowVolumeSpaceBar();
	fUsedSpaceColor = settings.UsedSpaceColor();
	fFreeSpaceColor = settings.FreeSpaceColor();
	fWarningSpaceColor = settings.WarningSpaceColor();
}


bool
SpaceBarSettingsView::IsRevertable() const
{
	TrackerSettings settings;

	return fSpaceBarShow != settings.ShowVolumeSpaceBar()
		|| fUsedSpaceColor != settings.UsedSpaceColor()
		|| fFreeSpaceColor != settings.FreeSpaceColor()
		|| fWarningSpaceColor != settings.WarningSpaceColor();
}
