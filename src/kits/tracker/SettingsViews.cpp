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

#include <Alert.h>

#include "Commands.h"
#include "DeskWindow.h"
#include "Model.h"
#include "SettingsViews.h"
#include "Tracker.h"
#include "WidgetAttributeText.h"

#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <GroupLayoutBuilder.h>
#include <Locale.h>
#include <MenuField.h>
#include <ColorControl.h>
#include <NodeMonitor.h>
#include <StringView.h>


static const uint32 kSpaceBarSwitchColor = 'SBsc';
static const float kItemExtraSpacing = 2.0f;
static const float kIndentSpacing = 12.0f;

//TODO: defaults should be set in one place only (TrackerSettings.cpp) while
//		being accessible from here.
//      What about adding DefaultValue(), IsDefault() etc... methods to xxxValueSetting ?
static const uint8 kSpaceBarAlpha = 192;
static const rgb_color kDefaultUsedSpaceColor = {0, 203, 0, kSpaceBarAlpha};
static const rgb_color kDefaultFreeSpaceColor = {255, 255, 255, kSpaceBarAlpha};
static const rgb_color kDefaultWarningSpaceColor = {203, 0, 0, kSpaceBarAlpha};


static void
send_bool_notices(uint32 what, const char *name, bool value)
{
	TTracker *tracker = dynamic_cast<TTracker *>(be_app);
	if (!tracker)
		return;

	BMessage message;
	message.AddBool(name, value);
	tracker->SendNotices(what, &message);
}


//	#pragma mark -


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SettingsView"

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


// #pragma mark -


DesktopSettingsView::DesktopSettingsView()
	:
	SettingsView("DesktopSettingsView")
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

	fMountButton = new BButton("",
		B_TRANSLATE("Mount settings" B_UTF8_ELLIPSIS),
		new BMessage(kRunAutomounterSettings));

	const float spacing = be_control_look->DefaultItemSpacing();

	BGroupLayoutBuilder(this)
		.AddGroup(B_VERTICAL, 0)
			.Add(fShowDisksIconRadioButton)
			.Add(fMountVolumesOntoDesktopRadioButton)
			.AddGroup(B_VERTICAL, 0)
				.Add(fMountSharedVolumesOntoDesktopCheckBox)
				.SetInsets(20, 0, 0, 0)
			.End()
			.AddGlue()
			.AddGroup(B_HORIZONTAL)
				.Add(fMountButton)
				.AddGlue()
			.End()
		.End()
		.SetInsets(spacing, spacing, spacing, spacing);

	fMountButton->SetTarget(be_app);
}


void
DesktopSettingsView::AttachedToWindow()
{
	fShowDisksIconRadioButton->SetTarget(this);
	fMountVolumesOntoDesktopRadioButton->SetTarget(this);
	fMountSharedVolumesOntoDesktopCheckBox->SetTarget(this);
}


void
DesktopSettingsView::MessageReceived(BMessage *message)
{
	TTracker *tracker = dynamic_cast<TTracker *>(be_app);
	if (!tracker)
		return;

	TrackerSettings settings;

	switch (message->what) {
		case kShowDisksIconChanged:
		{
			// Turn on and off related settings:
			fMountVolumesOntoDesktopRadioButton->SetValue(
				!fShowDisksIconRadioButton->Value() == 1);
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
			tracker->SendNotices(kVolumesOnDesktopChanged, &notificationMessage);

			// Tell the settings window the contents have changed:
			Window()->PostMessage(kSettingsContentsModified);
			break;
		}

		case kVolumesOnDesktopChanged:
		{
			// Turn on and off related settings:
			fShowDisksIconRadioButton->SetValue(
				!fMountVolumesOntoDesktopRadioButton->Value() == 1);
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
			tracker->SendNotices(kVolumesOnDesktopChanged, &notificationMessage);

			// Tell the settings window the contents have changed:
			Window()->PostMessage(kSettingsContentsModified);
			break;
		}

		default:
			_inherited::MessageReceived(message);
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
	TTracker *tracker = dynamic_cast<TTracker *>(be_app);
	if (!tracker)
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
	fMountVolumesOntoDesktopRadioButton->SetValue(settings.MountVolumesOntoDesktop());

	fMountSharedVolumesOntoDesktopCheckBox->SetValue(settings.MountSharedVolumesOntoDesktop());
	fMountSharedVolumesOntoDesktopCheckBox->SetEnabled(settings.MountVolumesOntoDesktop());
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


// #pragma mark -


WindowsSettingsView::WindowsSettingsView()
	:
	SettingsView("WindowsSettingsView")
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

	fTypeAheadFilteringCheckBox = new BCheckBox("",
		B_TRANSLATE("Enable type-ahead filtering"),
		new BMessage(kTypeAheadFilteringChanged));

	const float spacing = be_control_look->DefaultItemSpacing();

	BGroupLayoutBuilder(this)
		.AddGroup(B_VERTICAL, 0)
			.AddGroup(B_VERTICAL, 0)
				.Add(fShowFullPathInTitleBarCheckBox)
				.Add(fSingleWindowBrowseCheckBox)
			.End()
			.AddGroup(B_VERTICAL)
				.Add(fShowNavigatorCheckBox)
				.SetInsets(20, 0, 0, 0)
			.End()
			.AddGroup(B_VERTICAL, 0)
				.Add(fOutlineSelectionCheckBox)
				.Add(fSortFolderNamesFirstCheckBox)
				.Add(fTypeAheadFilteringCheckBox)
			.End()
		.AddGlue()
		.End()
		.SetInsets(spacing, spacing, spacing, spacing);
}


void
WindowsSettingsView::AttachedToWindow()
{
	fSingleWindowBrowseCheckBox->SetTarget(this);
	fShowNavigatorCheckBox->SetTarget(this);
	fShowFullPathInTitleBarCheckBox->SetTarget(this);
	fOutlineSelectionCheckBox->SetTarget(this);
	fSortFolderNamesFirstCheckBox->SetTarget(this);
	fTypeAheadFilteringCheckBox->SetTarget(this);
}


void
WindowsSettingsView::MessageReceived(BMessage *message)
{
	TTracker *tracker = dynamic_cast<TTracker *>(be_app);
	if (!tracker)
		return;
	TrackerSettings settings;

	switch (message->what) {
		case kWindowsShowFullPathChanged:
			settings.SetShowFullPathInTitleBar(fShowFullPathInTitleBarCheckBox->Value() == 1);
			tracker->SendNotices(kWindowsShowFullPathChanged);
			Window()->PostMessage(kSettingsContentsModified);
			break;

		case kSingleWindowBrowseChanged:
			settings.SetSingleWindowBrowse(fSingleWindowBrowseCheckBox->Value() == 1);
			if (fSingleWindowBrowseCheckBox->Value() == 0) {
				fShowNavigatorCheckBox->SetEnabled(false);
				settings.SetShowNavigator(0);
			} else {
				fShowNavigatorCheckBox->SetEnabled(true);
				settings.SetShowNavigator(fShowNavigatorCheckBox->Value() != 0);
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
			settings.SetSortFolderNamesFirst(fSortFolderNamesFirstCheckBox->Value() == 1);

			// Make the notification message and send it to the tracker:
			send_bool_notices(kSortFolderNamesFirstChanged, "SortFolderNamesFirst",
				fSortFolderNamesFirstCheckBox->Value() == 1);

			Window()->PostMessage(kSettingsContentsModified);
			break;
		}

		case kTypeAheadFilteringChanged:
		{
			settings.SetTypeAheadFiltering(fTypeAheadFilteringCheckBox->Value() == 1);
			send_bool_notices(kTypeAheadFilteringChanged, "TypeAheadFiltering",
				fTypeAheadFilteringCheckBox->Value() == 1);
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
	TTracker *tracker = dynamic_cast<TTracker *>(be_app);
	if (!tracker)
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

	if (settings.TypeAheadFiltering()) {
		settings.SetTypeAheadFiltering(false);
		send_bool_notices(kTypeAheadFilteringChanged,
			"TypeAheadFiltering", true);
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
		|| settings.TypeAheadFiltering() != false;
}


void
WindowsSettingsView::Revert()
{
	TTracker *tracker = dynamic_cast<TTracker *>(be_app);
	if (!tracker)
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

	if (settings.TypeAheadFiltering() != fTypeAheadFiltering) {
		settings.SetTypeAheadFiltering(fTypeAheadFiltering);
		send_bool_notices(kTypeAheadFilteringChanged,
			"TypeAheadFiltering", fTypeAheadFiltering);
	}

	ShowCurrentSettings();
}


void
WindowsSettingsView::ShowCurrentSettings()
{
	TrackerSettings settings;

	fShowFullPathInTitleBarCheckBox->SetValue(settings.ShowFullPathInTitleBar());
	fSingleWindowBrowseCheckBox->SetValue(settings.SingleWindowBrowse());
	fShowNavigatorCheckBox->SetEnabled(settings.SingleWindowBrowse());
	fShowNavigatorCheckBox->SetValue(settings.ShowNavigator());
	fOutlineSelectionCheckBox->SetValue(settings.TransparentSelection()
		? B_CONTROL_OFF : B_CONTROL_ON);
	fSortFolderNamesFirstCheckBox->SetValue(settings.SortFolderNamesFirst());
	fTypeAheadFilteringCheckBox->SetValue(settings.TypeAheadFiltering());
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
	fTypeAheadFiltering = settings.TypeAheadFiltering();
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
		|| fTypeAheadFiltering != settings.TypeAheadFiltering();
}


// #pragma mark -


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

	BBox* box = new BBox("box");
	box->SetLabel(fColorPicker = new BMenuField("menu", NULL, menu));

	fColorControl = new BColorControl(BPoint(8, fColorPicker->Bounds().Height()
			+ 8 + kItemExtraSpacing),
		B_CELLS_16x16, 1, "SpaceColorControl", new BMessage(kSpaceBarColorChanged));
	fColorControl->SetValue(TrackerSettings().UsedSpaceColor());
	box->AddChild(fColorControl);

	const float spacing = be_control_look->DefaultItemSpacing();

	BGroupLayout* layout = GroupLayout();
	layout->SetOrientation(B_VERTICAL);
	layout->SetSpacing(0);
	BGroupLayoutBuilder(layout)
		.Add(fSpaceBarShowCheckBox)
		.Add(box)
		.AddGlue()
		.SetInsets(spacing, spacing, spacing, spacing);

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
SpaceBarSettingsView::MessageReceived(BMessage *message)
{
	TTracker *tracker = dynamic_cast<TTracker *>(be_app);
	if (!tracker)
		return;
	TrackerSettings settings;

	switch (message->what) {
		case kUpdateVolumeSpaceBar:
		{
			settings.SetShowVolumeSpaceBar(fSpaceBarShowCheckBox->Value() == 1);
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
				//alpha is ignored by BColorControl but is checked in equalities

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

			Window()->PostMessage(kSettingsContentsModified);
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
	TTracker *tracker = dynamic_cast<TTracker *>(be_app);
	if (!tracker)
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
	TTracker *tracker = dynamic_cast<TTracker *>(be_app);
	if (!tracker)
		return;

	TrackerSettings settings;

	if (settings.ShowVolumeSpaceBar() != fSpaceBarShow) {
		settings.SetShowVolumeSpaceBar(fSpaceBarShow);
		send_bool_notices(kShowVolumeSpaceBar, "ShowVolumeSpaceBar", fSpaceBarShow);
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


// #pragma mark -


TrashSettingsView::TrashSettingsView()
	:
	SettingsView("TrashSettingsView")
{
	fDontMoveFilesToTrashCheckBox = new BCheckBox("",
		B_TRANSLATE("Don't move files to Trash"),
			new BMessage(kDontMoveFilesToTrashChanged));

	fAskBeforeDeleteFileCheckBox = new BCheckBox("",
		B_TRANSLATE("Ask before delete"),
			new BMessage(kAskBeforeDeleteFileChanged));

	const float spacing = be_control_look->DefaultItemSpacing();

	BGroupLayout* layout = GroupLayout();
	layout->SetOrientation(B_VERTICAL);
	layout->SetSpacing(0);
	BGroupLayoutBuilder(layout)
		.Add(fDontMoveFilesToTrashCheckBox)
		.Add(fAskBeforeDeleteFileCheckBox)
		.AddGlue()
		.SetInsets(spacing, spacing, spacing, spacing);

}


void
TrashSettingsView::AttachedToWindow()
{
	fDontMoveFilesToTrashCheckBox->SetTarget(this);
	fAskBeforeDeleteFileCheckBox->SetTarget(this);
}


void
TrashSettingsView::MessageReceived(BMessage *message)
{
	TTracker *tracker = dynamic_cast<TTracker *>(be_app);
	if (!tracker)
		return;
	TrackerSettings settings;

	switch (message->what) {
		case kDontMoveFilesToTrashChanged:
			settings.SetDontMoveFilesToTrash(fDontMoveFilesToTrashCheckBox->Value() == 1);

			tracker->SendNotices(kDontMoveFilesToTrashChanged);
			Window()->PostMessage(kSettingsContentsModified);
			break;

		case kAskBeforeDeleteFileChanged:
			settings.SetAskBeforeDeleteFile(fAskBeforeDeleteFileCheckBox->Value() == 1);

			tracker->SendNotices(kAskBeforeDeleteFileChanged);
			Window()->PostMessage(kSettingsContentsModified);
			break;

		default:
			_inherited::MessageReceived(message);
			break;
	}
}


void
TrashSettingsView::SetDefaults()
{
	TrackerSettings settings;

	settings.SetDontMoveFilesToTrash(false);
	settings.SetAskBeforeDeleteFile(true);

	ShowCurrentSettings();
	_SendNotices();
}


bool
TrashSettingsView::IsDefaultable() const
{
	TrackerSettings settings;

	return settings.DontMoveFilesToTrash() != false
		|| settings.AskBeforeDeleteFile() != true;
}


void
TrashSettingsView::Revert()
{
	TrackerSettings settings;

	settings.SetDontMoveFilesToTrash(fDontMoveFilesToTrash);
	settings.SetAskBeforeDeleteFile(fAskBeforeDeleteFile);

	ShowCurrentSettings();
	_SendNotices();
}


void
TrashSettingsView::_SendNotices()
{
	TTracker *tracker = dynamic_cast<TTracker *>(be_app);
	if (!tracker)
		return;

	tracker->SendNotices(kDontMoveFilesToTrashChanged);
	tracker->SendNotices(kAskBeforeDeleteFileChanged);
}


void
TrashSettingsView::ShowCurrentSettings()
{
	TrackerSettings settings;

	fDontMoveFilesToTrashCheckBox->SetValue(settings.DontMoveFilesToTrash());
	fAskBeforeDeleteFileCheckBox->SetValue(settings.AskBeforeDeleteFile());
}


void
TrashSettingsView::RecordRevertSettings()
{
	TrackerSettings settings;

	fDontMoveFilesToTrash = settings.DontMoveFilesToTrash();
	fAskBeforeDeleteFile = settings.AskBeforeDeleteFile();
}


bool
TrashSettingsView::IsRevertable() const
{
	return fDontMoveFilesToTrash != (fDontMoveFilesToTrashCheckBox->Value() > 0)
		|| fAskBeforeDeleteFile != (fAskBeforeDeleteFileCheckBox->Value() > 0);
}

