/*
 * Copyright 2003-2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Jérôme Duval, jerome.duval@free.fr
 *		Filip Maryjański, widelec@morphos.pl
 *		Puck Meerburg, puck@puckipedia.nl
 *		Michael Phipps
 *		John Scipione, jscipione@gmail.com
 */


#include "ScreenSaverWindow.h"

#include <stdio.h>
#include <strings.h>

#include <Alignment.h>
#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <DefaultSettingsView.h>
#include <Directory.h>
#include <DurationFormat.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Font.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <ListItem.h>
#include <ListView.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <Rect.h>
#include <Roster.h>
#include <Screen.h>
#include <ScreenSaver.h>
#include <ScreenSaverRunner.h>
#include <ScrollView.h>
#include <Size.h>
#include <Slider.h>
#include <StringView.h>
#include <TabView.h>
#include <TextView.h>

#include <algorithm>
	// for std::max and std::min

#include "PreviewView.h"
#include "ScreenCornerSelector.h"
#include "ScreenSaverItem.h"
#include "ScreenSaverShared.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ScreenSaver"


const uint32 kPreviewMonitorGap = 16;
const uint32 kMinSettingsWidth = 230;
const uint32 kMinSettingsHeight = 120;

const int32 kMsgSaverSelected = 'SSEL';
const int32 kMsgPasswordCheckBox = 'PWCB';
const int32 kMsgRunSliderChanged = 'RSch';
const int32 kMsgRunSliderUpdate = 'RSup';
const int32 kMsgPasswordSliderChanged = 'PWch';
const int32 kMsgPasswordSliderUpdate = 'PWup';
const int32 kMsgChangePassword = 'PWBT';
const int32 kMsgEnableScreenSaverBox = 'ESCH';

const int32 kMsgTurnOffCheckBox = 'TUOF';
const int32 kMsgTurnOffSliderChanged = 'TUch';
const int32 kMsgTurnOffSliderUpdate = 'TUup';

const int32 kMsgFadeCornerChanged = 'fdcc';
const int32 kMsgNeverFadeCornerChanged = 'nfcc';

const float kWindowWidth = 446.0f;
const float kWindowHeight = 325.0f;
const float kDefaultItemSpacingAt12pt = 12.0f * 0.85;


class TimeSlider : public BSlider {
public:
								TimeSlider(const char* name,
									uint32 changedMessage,
									uint32 updateMessage);
	virtual						~TimeSlider();

	virtual	void				SetValue(int32 value);

			void				SetTime(bigtime_t useconds);
			bigtime_t			Time() const;

private:
			void				_TimeToString(bigtime_t useconds,
									BString& string);
};


class TabView : public BTabView {
public:
								TabView();

	virtual	void				MouseDown(BPoint where);
};


class FadeView : public BView {
public:
								FadeView(const char* name,
									ScreenSaverSettings& settings);

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

			void				UpdateTurnOffScreen();
			void				UpdateStatus();

private:
			void				_UpdateColors();
			ScreenSaverSettings&	fSettings;
			uint32				fTurnOffScreenFlags;

			BCheckBox*			fEnableCheckBox;
			TimeSlider*			fRunSlider;

			BTextView*			fTurnOffNotSupported;
			BCheckBox*			fTurnOffCheckBox;
			TimeSlider*			fTurnOffSlider;

			BTextView*			fFadeNeverText;
			BTextView*			fFadeNowText;

			BCheckBox*			fPasswordCheckBox;
			TimeSlider*			fPasswordSlider;
			BButton*			fPasswordButton;

			ScreenCornerSelector*	fFadeNow;
			ScreenCornerSelector*	fFadeNever;
};


class ModulesView : public BView {
public:
								ModulesView(const char* name,
									ScreenSaverSettings& settings);
	virtual						~ModulesView();

	virtual	void				DetachedFromWindow();
	virtual	void				AttachedToWindow();
	virtual	void				AllAttached();
	virtual	void				MessageReceived(BMessage* message);

			void				EmptyScreenSaverList();
			void				PopulateScreenSaverList();

			void				SaveState();

			BScreenSaver*		ScreenSaver();

private:
	friend class TabView;

	static	int					_CompareScreenSaverItems(const void* left,
									const void* right);

			void				_CloseSaver();
			void				_OpenSaver();
			void				_AddNewScreenSaverToList(const char* name,
								BPath* path);
			void				_RemoveScreenSaverFromList(const char* name);

private:
		ScreenSaverSettings&	fSettings;

			BListView*			fScreenSaversListView;
			BButton*			fTestButton;

			ScreenSaverRunner*	fSaverRunner;
			BString				fCurrentName;

			BBox*				fSettingsBox;
			BView*				fSettingsView;

			PreviewView*		fPreviewView;

			team_id				fScreenSaverTestTeam;
};


//	#pragma mark - TimeSlider


static const int32 kTimeInUnits[] = {
	30,    60,   90,
	120,   150,  180,
	240,   300,  360,
	420,   480,  540,
	600,   900,  1200,
	1500,  1800, 2400,
	3000,  3600, 5400,
	7200,  9000, 10800,
	14400, 18000
};

static const int32 kTimeUnitCount
	= sizeof(kTimeInUnits) / sizeof(kTimeInUnits[0]);


TimeSlider::TimeSlider(const char* name, uint32 changedMessage,
	uint32 updateMessage)
	:
	BSlider(name, B_TRANSLATE("30 seconds"), new BMessage(changedMessage),
		0, kTimeUnitCount - 1, B_HORIZONTAL, B_TRIANGLE_THUMB)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	SetModificationMessage(new BMessage(updateMessage));
	SetBarThickness(10);
}


TimeSlider::~TimeSlider()
{
}


void
TimeSlider::SetValue(int32 value)
{
	int32 oldValue = Value();
	BSlider::SetValue(value);

	if (oldValue != Value()) {
		BString label;
		_TimeToString(kTimeInUnits[Value()] * 1000000LL, label);
		SetLabel(label.String());
	}
}


void
TimeSlider::SetTime(bigtime_t useconds)
{
	for (int t = 0; t < kTimeUnitCount; t++) {
		if (kTimeInUnits[t] * 1000000LL == useconds) {
			SetValue(t);
			break;
		}
	}
}


bigtime_t
TimeSlider::Time() const
{
	return 1000000LL * kTimeInUnits[Value()];
}


void
TimeSlider::_TimeToString(bigtime_t useconds, BString& string)
{
	BDurationFormat formatter;
	formatter.Format(string, 0, useconds);
}


//	#pragma mark - FadeView


FadeView::FadeView(const char* name, ScreenSaverSettings& settings)
	:
	BView(name, B_WILL_DRAW),
	fSettings(settings)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	font_height fontHeight;
	be_plain_font->GetHeight(&fontHeight);
	float textHeight = ceilf(fontHeight.ascent + fontHeight.descent);

	fEnableCheckBox = new BCheckBox("EnableCheckBox",
		B_TRANSLATE("Enable screensaver"),
		new BMessage(kMsgEnableScreenSaverBox));

	BBox* box = new BBox("EnableScreenSaverBox");
	box->SetLabel(fEnableCheckBox);

	// Start Screensaver
	BStringView* startScreenSaver = new BStringView("startScreenSaver",
		B_TRANSLATE("Start screensaver"));
	startScreenSaver->SetAlignment(B_ALIGN_RIGHT);

	fRunSlider = new TimeSlider("RunSlider", kMsgRunSliderChanged,
		kMsgRunSliderUpdate);

	// Turn Off
	rgb_color textColor = disable_color(ui_color(B_PANEL_TEXT_COLOR),
		ViewColor());

	fTurnOffNotSupported = new BTextView("not_supported", be_plain_font,
		&textColor, B_WILL_DRAW);
	fTurnOffNotSupported->SetExplicitMinSize(BSize(B_SIZE_UNSET,
		3 + textHeight * 3));
	fTurnOffNotSupported->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	fTurnOffNotSupported->MakeEditable(false);
	fTurnOffNotSupported->MakeSelectable(false);
	fTurnOffNotSupported->SetText(
		B_TRANSLATE("Display Power Management Signaling not available"));

	fTurnOffCheckBox = new BCheckBox("TurnOffScreenCheckBox",
		B_TRANSLATE("Turn off screen"), new BMessage(kMsgTurnOffCheckBox));
	fTurnOffCheckBox->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_CENTER));

	fTurnOffSlider = new TimeSlider("TurnOffSlider", kMsgTurnOffSliderChanged,
		kMsgTurnOffSliderUpdate);

	// Password
	fPasswordCheckBox = new BCheckBox("PasswordCheckbox",
		B_TRANSLATE("Password lock"), new BMessage(kMsgPasswordCheckBox));
	fPasswordCheckBox->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_CENTER));

	fPasswordSlider = new TimeSlider("PasswordSlider",
		kMsgPasswordSliderChanged, kMsgPasswordSliderUpdate);

	fPasswordButton = new BButton("PasswordButton",
		B_TRANSLATE("Password" B_UTF8_ELLIPSIS),
		new BMessage(kMsgChangePassword));

	// Bottom
	float monitorHeight = 10 + textHeight * 3;
	float aspectRatio = 4.0f / 3.0f;
	float monitorWidth = monitorHeight * aspectRatio;
	BRect monitorRect = BRect(0, 0, monitorWidth, monitorHeight);

	fFadeNow = new ScreenCornerSelector(monitorRect, "FadeNow",
		new BMessage(kMsgFadeCornerChanged), B_FOLLOW_NONE);
	fFadeNowText = new BTextView("FadeNowText", B_WILL_DRAW);
	fFadeNowText->SetExplicitMinSize(BSize(B_SIZE_UNSET,
		4 + textHeight * 4));
	fFadeNowText->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	fFadeNowText->MakeEditable(false);
	fFadeNowText->MakeSelectable(false);
	fFadeNowText->SetText(B_TRANSLATE("Fade now when mouse is here"));

	fFadeNever = new ScreenCornerSelector(monitorRect, "FadeNever",
		new BMessage(kMsgNeverFadeCornerChanged), B_FOLLOW_NONE);
	fFadeNeverText = new BTextView("FadeNeverText", B_WILL_DRAW);
	fFadeNeverText->SetExplicitMinSize(BSize(B_SIZE_UNSET,
		4 + textHeight * 4));
	fFadeNeverText->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	fFadeNeverText->MakeEditable(false);
	fFadeNeverText->MakeSelectable(false);
	fFadeNeverText->SetText(B_TRANSLATE("Don't fade when mouse is here"));

	box->AddChild(BLayoutBuilder::Group<>(B_VERTICAL, 0)
		.SetInsets(B_USE_DEFAULT_SPACING, 0, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING)
		.AddGrid(B_USE_DEFAULT_SPACING, B_USE_SMALL_SPACING)
			.Add(startScreenSaver, 0, 0)
			.Add(fRunSlider, 1, 0)
			.Add(fTurnOffCheckBox, 0, 1)
			.Add(BLayoutBuilder::Group<>(B_VERTICAL)
				.Add(fTurnOffNotSupported)
				.Add(fTurnOffSlider)
				.View(), 1, 1)
			.Add(fPasswordCheckBox, 0, 2)
			.Add(fPasswordSlider, 1, 2)
			.End()
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fPasswordButton)
			.End()
		.AddGlue()
		.AddGroup(B_HORIZONTAL)
			.Add(fFadeNow)
			.AddGroup(B_VERTICAL, 0)
				.Add(fFadeNowText)
				.AddGlue()
				.End()
			.Add(fFadeNever)
			.AddGroup(B_VERTICAL, 0)
				.Add(fFadeNeverText)
				.AddGlue()
				.End()
			.End()
		.AddGlue()
		.View());

	BLayoutBuilder::Group<>(this, B_HORIZONTAL)
		.SetInsets(B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING,
			B_USE_WINDOW_SPACING, 0)
		.Add(box)
		.End();

}


void
FadeView::AttachedToWindow()
{
	fEnableCheckBox->SetTarget(this);
	fRunSlider->SetTarget(this);
	fTurnOffCheckBox->SetTarget(this);
	fTurnOffSlider->SetTarget(this);
	fFadeNow->SetTarget(this);
	fFadeNever->SetTarget(this);
	fPasswordCheckBox->SetTarget(this);
	fPasswordSlider->SetTarget(this);

	fEnableCheckBox->SetValue(
		fSettings.TimeFlags() & ENABLE_SAVER ? B_CONTROL_ON : B_CONTROL_OFF);
	fRunSlider->SetTime(fSettings.BlankTime());
	fTurnOffSlider->SetTime(fSettings.OffTime() + fSettings.BlankTime());
	fFadeNow->SetCorner(fSettings.BlankCorner());
	fFadeNever->SetCorner(fSettings.NeverBlankCorner());
	fPasswordCheckBox->SetValue(fSettings.LockEnable());
	fPasswordSlider->SetTime(fSettings.PasswordTime());

	_UpdateColors();
	UpdateTurnOffScreen();
	UpdateStatus();
}


void
FadeView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_COLORS_UPDATED:
			_UpdateColors();
			break;
		case kMsgRunSliderChanged:
		case kMsgRunSliderUpdate:
			if (fRunSlider->Value() > fTurnOffSlider->Value())
				fTurnOffSlider->SetValue(fRunSlider->Value());

			if (fRunSlider->Value() > fPasswordSlider->Value())
				fPasswordSlider->SetValue(fRunSlider->Value());
			break;

		case kMsgTurnOffSliderChanged:
		case kMsgTurnOffSliderUpdate:
			if (fRunSlider->Value() > fTurnOffSlider->Value())
				fRunSlider->SetValue(fTurnOffSlider->Value());
			break;

		case kMsgPasswordSliderChanged:
		case kMsgPasswordSliderUpdate:
			if (fPasswordSlider->Value() < fRunSlider->Value())
				fRunSlider->SetValue(fPasswordSlider->Value());
			break;

		case kMsgTurnOffCheckBox:
			fTurnOffSlider->SetEnabled(
				fTurnOffCheckBox->Value() == B_CONTROL_ON);
			break;
	}

	switch (message->what) {
		case kMsgRunSliderChanged:
		case kMsgTurnOffSliderChanged:
		case kMsgPasswordSliderChanged:
		case kMsgPasswordCheckBox:
		case kMsgEnableScreenSaverBox:
		case kMsgFadeCornerChanged:
		case kMsgNeverFadeCornerChanged:
			UpdateStatus();
			fSettings.Save();
			break;

		default:
			BView::MessageReceived(message);
	}
}


void
FadeView::UpdateTurnOffScreen()
{
	bool enabled = (fSettings.TimeFlags() & ENABLE_DPMS_MASK) != 0;

	BScreen screen(Window());
	uint32 dpmsCapabilities = screen.DPMSCapabilites();

	fTurnOffScreenFlags = 0;
	if (dpmsCapabilities & B_DPMS_OFF)
		fTurnOffScreenFlags |= ENABLE_DPMS_OFF;
	if (dpmsCapabilities & B_DPMS_STAND_BY)
		fTurnOffScreenFlags |= ENABLE_DPMS_STAND_BY;
	if (dpmsCapabilities & B_DPMS_SUSPEND)
		fTurnOffScreenFlags |= ENABLE_DPMS_SUSPEND;

	fTurnOffCheckBox->SetValue(enabled && fTurnOffScreenFlags != 0
		? B_CONTROL_ON : B_CONTROL_OFF);

	enabled = fEnableCheckBox->Value() == B_CONTROL_ON;
	fTurnOffCheckBox->SetEnabled(enabled && fTurnOffScreenFlags != 0);
	if (fTurnOffScreenFlags != 0) {
		fTurnOffNotSupported->Hide();
		fTurnOffSlider->Show();
	} else {
		fTurnOffSlider->Hide();
		fTurnOffNotSupported->Show();
	}
}


void
FadeView::UpdateStatus()
{
	Window()->DisableUpdates();

	bool enabled = fEnableCheckBox->Value() == B_CONTROL_ON;
	fPasswordCheckBox->SetEnabled(enabled);
	fTurnOffCheckBox->SetEnabled(enabled && fTurnOffScreenFlags != 0);
	fRunSlider->SetEnabled(enabled);
	fTurnOffSlider->SetEnabled(enabled && fTurnOffCheckBox->Value());
	fPasswordSlider->SetEnabled(enabled && fPasswordCheckBox->Value());
	fPasswordButton->SetEnabled(enabled && fPasswordCheckBox->Value());

	Window()->EnableUpdates();

	// Update the saved preferences
	fSettings.SetWindowFrame(Frame());
	fSettings.SetTimeFlags((enabled ? ENABLE_SAVER : 0)
		| (fTurnOffCheckBox->Value() ? fTurnOffScreenFlags : 0));
	fSettings.SetBlankTime(fRunSlider->Time());
	bigtime_t offTime = fTurnOffSlider->Time() - fSettings.BlankTime();
	fSettings.SetOffTime(offTime);
	fSettings.SetSuspendTime(offTime);
	fSettings.SetStandByTime(offTime);
	fSettings.SetBlankCorner(fFadeNow->Corner());
	fSettings.SetNeverBlankCorner(fFadeNever->Corner());
	fSettings.SetLockEnable(fPasswordCheckBox->Value());
	fSettings.SetPasswordTime(fPasswordSlider->Time());

	// TODO - Tell the password window to update its stuff
}


void
FadeView::_UpdateColors()
{
	rgb_color color = ui_color(B_PANEL_TEXT_COLOR);
	fFadeNeverText->SetFontAndColor(be_plain_font, 0, &color);
	fFadeNowText->SetFontAndColor(be_plain_font, 0, &color);
}


//	#pragma mark - ModulesView


ModulesView::ModulesView(const char* name, ScreenSaverSettings& settings)
	:
	BView(name, B_WILL_DRAW),
	fSettings(settings),
	fScreenSaversListView(new BListView("SaversListView")),
	fTestButton(new BButton("TestButton", B_TRANSLATE("Test"),
		new BMessage(kMsgTestSaver))),
	fSaverRunner(NULL),
	fSettingsBox(new BBox("SettingsBox")),
	fSettingsView(NULL),
	fPreviewView(new PreviewView("preview")),
	fScreenSaverTestTeam(-1)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	fScreenSaversListView->SetSelectionMessage(
		new BMessage(kMsgSaverSelected));
	fScreenSaversListView->SetInvocationMessage(
		new BMessage(kMsgTestSaver));
	BScrollView* saversListScrollView = new BScrollView("scroll_list",
		fScreenSaversListView, 0, false, true);

	fSettingsBox->SetLabel(B_TRANSLATE("Screensaver settings"));
	fSettingsBox->SetExplicitMinSize(BSize(
		floorf(be_control_look->DefaultItemSpacing()
			* ((kWindowWidth - 157.0f) / kDefaultItemSpacingAt12pt)),
		B_SIZE_UNSET));

	BLayoutBuilder::Group<>(this, B_HORIZONTAL)
		.SetInsets(B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING,
			B_USE_WINDOW_SPACING, 0)
		.AddGroup(B_VERTICAL)
			.Add(fPreviewView)
			.Add(saversListScrollView)
			.Add(fTestButton)
			.End()
		.Add(fSettingsBox)
		.End();
}


ModulesView::~ModulesView()
{
	stop_watching(this);

	delete fTestButton;
	delete fSettingsBox;
	delete fPreviewView;
}


void
ModulesView::DetachedFromWindow()
{
	SaveState();
	EmptyScreenSaverList();

	_CloseSaver();
}


void
ModulesView::AttachedToWindow()
{
	fScreenSaversListView->SetTarget(this);
	fTestButton->SetTarget(this);
}


void
ModulesView::AllAttached()
{
	PopulateScreenSaverList();
}


void
ModulesView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgSaverSelected:
		{
			int32 selection = fScreenSaversListView->CurrentSelection();
			if (selection < 0)
				break;

			ScreenSaverItem* item
				= (ScreenSaverItem*)fScreenSaversListView->ItemAt(selection);
			if (item == NULL)
				break;

			if (strcmp(item->Text(), B_TRANSLATE("Blackness")) == 0)
				fSettings.SetModuleName("");
			else
				fSettings.SetModuleName(item->Text());

			SaveState();
			_CloseSaver();
			_OpenSaver();
			fSettings.Save();
			break;
		}

		case kMsgTestSaver:
		{
			SaveState();
			fSettings.Save();

			_CloseSaver();

			be_roster->StartWatching(BMessenger(this, Looper()),
				B_REQUEST_QUIT);
			BMessage message(kMsgTestSaver);
			if (be_roster->Launch(SCREEN_BLANKER_SIG, &message,
					&fScreenSaverTestTeam) == B_OK) {
				break;
			}

			// Try really hard to launch it. It's very likely that this fails
			// when we run from the CD, and there is only an incomplete mime
			// database for example...
			BPath path;
			if (find_directory(B_SYSTEM_BIN_DIRECTORY, &path) != B_OK
				|| path.Append("screen_blanker") != B_OK) {
				path.SetTo("/bin/screen_blanker");
			}

			BEntry entry(path.Path());
			entry_ref ref;
			if (entry.GetRef(&ref) == B_OK) {
				be_roster->Launch(&ref, &message,
					&fScreenSaverTestTeam);
			}
			break;
		}

		case B_NODE_MONITOR:
		{
			switch (message->GetInt32("opcode", 0)) {
				case B_ENTRY_CREATED:
				{
					const char* name;
					node_ref nodeRef;

					message->FindString("name", &name);
					message->FindInt32("device", &nodeRef.device);
					message->FindInt64("directory", &nodeRef.node);

					BDirectory dir(&nodeRef);

					if (dir.InitCheck() == B_OK) {
						BPath path(&dir);
						_AddNewScreenSaverToList(name, &path);
					}
					break;
				}


				case B_ENTRY_MOVED:
				case B_ENTRY_REMOVED:
				{
					const char* name;

					message->FindString("name", &name);
					_RemoveScreenSaverFromList(name);

					break;
				}

				default:
					// ignore any other operations
					break;
			}
			break;
		}

		case B_SOME_APP_QUIT:
		{
			team_id team;
			if (message->FindInt32("be:team", &team) == B_OK
				&& team == fScreenSaverTestTeam) {
				be_roster->StopWatching(this);
				_OpenSaver();
			}
			break;
		}

		default:
			BView::MessageReceived(message);
	}
}


void
ModulesView::SaveState()
{
	BScreenSaver* saver = ScreenSaver();
	if (saver == NULL)
		return;

	BMessage state;
	if (saver->SaveState(&state) == B_OK)
		fSettings.SetModuleState(fCurrentName.String(), &state);
}


void
ModulesView::EmptyScreenSaverList()
{
	fScreenSaversListView->DeselectAll();
	while (BListItem* item = fScreenSaversListView->RemoveItem((int32)0))
		delete item;
}


void
ModulesView::PopulateScreenSaverList()
{
	// Blackness is a built-in screen saver
	ScreenSaverItem* defaultItem
		= new ScreenSaverItem(B_TRANSLATE("Blackness"), "");
	fScreenSaversListView->AddItem(defaultItem);

	// Iterate over add-on directories, and add their files to the list view

	directory_which which[] = {
		B_USER_NONPACKAGED_ADDONS_DIRECTORY,
		B_USER_ADDONS_DIRECTORY,
		B_SYSTEM_NONPACKAGED_ADDONS_DIRECTORY,
		B_SYSTEM_ADDONS_DIRECTORY,
	};
	ScreenSaverItem* selectedItem = NULL;

	for (uint32 i = 0; i < sizeof(which) / sizeof(which[0]); i++) {
		BPath basePath;
		if (find_directory(which[i], &basePath) != B_OK)
			continue;
		else if (basePath.Append("Screen Savers", true) != B_OK)
			continue;

		BDirectory dir(basePath.Path());
		BEntry entry;
		node_ref nodeRef;

		dir.GetNodeRef(&nodeRef);
		watch_node(&nodeRef, B_WATCH_DIRECTORY, this);

		while (dir.GetNextEntry(&entry, true) == B_OK) {
			char name[B_FILE_NAME_LENGTH];
			if (entry.GetName(name) != B_OK)
				continue;

			BPath path(basePath);
			if (path.Append(name) != B_OK)
				continue;

			ScreenSaverItem* item = new ScreenSaverItem(name, path.Path());
			fScreenSaversListView->AddItem(item);

			if (selectedItem != NULL)
				continue;

			if (strcmp(fSettings.ModuleName(), item->Text()) == 0)
				selectedItem = item;
		}
	}

	fScreenSaversListView->SortItems(_CompareScreenSaverItems);
	if (selectedItem == NULL)
		selectedItem = defaultItem;

	fScreenSaversListView->Select(fScreenSaversListView->IndexOf(selectedItem));
	fScreenSaversListView->ScrollToSelection();
}


//! Sorting function for ScreenSaverItems
int
ModulesView::_CompareScreenSaverItems(const void* left, const void* right)
{
	ScreenSaverItem* leftItem  = *(ScreenSaverItem **)left;
	ScreenSaverItem* rightItem = *(ScreenSaverItem **)right;

	return strcasecmp(leftItem->Text(), rightItem->Text());
}


BScreenSaver*
ModulesView::ScreenSaver()
{
	if (fSaverRunner != NULL)
		return fSaverRunner->ScreenSaver();

	return NULL;
}


void
ModulesView::_CloseSaver()
{
	// remove old screen saver preview & config

	BScreenSaver* saver = ScreenSaver();
	BView* view = fPreviewView->RemovePreview();
	if (fSettingsView != NULL)
		fSettingsBox->RemoveChild(fSettingsView);

	if (fSaverRunner != NULL)
		fSaverRunner->Quit();

	if (saver != NULL)
		saver->StopConfig();

	delete view;
	delete fSettingsView;
	delete fSaverRunner;
		// the saver runner also unloads the add-on, so it must
		// be deleted last

	fSettingsView = NULL;
	fSaverRunner = NULL;
}


void
ModulesView::_OpenSaver()
{
	// create new screen saver preview & config

	BView* view = fPreviewView->AddPreview();
	fCurrentName = fSettings.ModuleName();
	fSaverRunner = new ScreenSaverRunner(view->Window(), view, fSettings);

#ifdef __HAIKU__
	BRect rect = fSettingsBox->InnerFrame().InsetByCopy(4, 4);
#else
	BRect rect = fSettingsBox->Bounds().InsetByCopy(4, 4);
	rect.top += 14;
#endif
	fSettingsView = new BView(rect, "SettingsView", B_FOLLOW_ALL, B_WILL_DRAW);

	fSettingsView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	fSettingsBox->AddChild(fSettingsView);

	BScreenSaver* saver = ScreenSaver();
	if (saver != NULL && fSettingsView != NULL) {
		saver->StartConfig(fSettingsView);
		if (saver->StartSaver(view, true) == B_OK) {
			fPreviewView->HideNoPreview();
			fSaverRunner->Run();
		} else
			fPreviewView->ShowNoPreview();
	} else {
		// Failed to load OR this is the "Blackness" screensaver. Show a black
		// preview (this is what will happen in both cases when screen_blanker
		// runs).
		fPreviewView->HideNoPreview();
	}

	if (fSettingsView->ChildAt(0) == NULL) {
		// There are no settings at all, we add the module name here to
		// let it look a bit better at least.
		BPrivate::BuildDefaultSettingsView(fSettingsView,
			fSettings.ModuleName()[0] ? fSettings.ModuleName()
				: B_TRANSLATE("Blackness"),
				saver != NULL || !fSettings.ModuleName()[0]
					? B_TRANSLATE("No options available")
					: B_TRANSLATE("Could not load screen saver"));
	}
}


void
ModulesView::_AddNewScreenSaverToList(const char* name, BPath* path)
{
	int32 oldSelected = fScreenSaversListView->CurrentSelection();
	ScreenSaverItem* selectedItem = (ScreenSaverItem*)fScreenSaversListView->ItemAt(
		oldSelected);

	path->Append(name);
	fScreenSaversListView->AddItem(new ScreenSaverItem(name, path->Path()));
	fScreenSaversListView->SortItems(_CompareScreenSaverItems);

	if (selectedItem != NULL) {
		fScreenSaversListView->Select(fScreenSaversListView->IndexOf(
			selectedItem));
		fScreenSaversListView->ScrollToSelection();
	}
}


void
ModulesView::_RemoveScreenSaverFromList(const char* name)
{
	int32 oldSelected = fScreenSaversListView->CurrentSelection();
	ScreenSaverItem* selectedItem = (ScreenSaverItem*)fScreenSaversListView->ItemAt(
		oldSelected);

	if (strcasecmp(selectedItem->Text(), name) == 0) {
		fScreenSaversListView->RemoveItem(selectedItem);
		fScreenSaversListView->SortItems(_CompareScreenSaverItems);
		fScreenSaversListView->Select(0);
		fScreenSaversListView->ScrollToSelection();
		return;
	}

	for (int i = 0, max = fScreenSaversListView->CountItems(); i < max; i++) {
		ScreenSaverItem* item = (ScreenSaverItem*)fScreenSaversListView->ItemAt(
			i);

		if (strcasecmp(item->Text(), name) == 0) {
			fScreenSaversListView->RemoveItem(item);
			delete item;
			break;
		}
	}

	fScreenSaversListView->SortItems(_CompareScreenSaverItems);

	oldSelected = fScreenSaversListView->IndexOf(selectedItem);
	fScreenSaversListView->Select(oldSelected);
	fScreenSaversListView->ScrollToSelection();
}


//	#pragma mark - TabView


TabView::TabView()
	:
	BTabView("tab_view", B_WIDTH_FROM_LABEL)
{
}


void
TabView::MouseDown(BPoint where)
{
	BTab* fadeTab = TabAt(0);
	BRect fadeTabFrame(TabFrame(0));
	BTab* modulesTab = TabAt(1);
	BRect modulesTabFrame(TabFrame(1));
	ModulesView* modulesView = NULL;

	if (modulesTab != NULL)
		modulesView = dynamic_cast<ModulesView*>(modulesTab->View());

	if (fadeTab != NULL && Selection() != 0 && fadeTabFrame.Contains(where)
		&& modulesView != NULL) {
		// clicked on the fade tab
		modulesView->SaveState();
		modulesView->_CloseSaver();
	} else if (modulesTab != NULL && Selection() != 1
		&& modulesTabFrame.Contains(where) && modulesView != NULL) {
		// clicked on the modules tab
		BMessage message(kMsgSaverSelected);
		modulesView->MessageReceived(&message);
	}

	BTabView::MouseDown(where);
}


//	#pragma mark - ScreenSaverWindow


ScreenSaverWindow::ScreenSaverWindow()
	:
	BWindow(BRect(50.0f, 50.0f, 50.0f + kWindowWidth, 50.0f + kWindowHeight),
		B_TRANSLATE_SYSTEM_NAME("ScreenSaver"), B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS)
{
	fSettings.Load();

	fMinWidth = floorf(be_control_look->DefaultItemSpacing()
		* (kWindowWidth / kDefaultItemSpacingAt12pt));

	font_height fontHeight;
	be_plain_font->GetHeight(&fontHeight);
	float textHeight = ceilf(fontHeight.ascent + fontHeight.descent);

	fMinHeight = ceilf(std::max(kWindowHeight, textHeight * 28));

	// Create the password editing window
	fPasswordWindow = new PasswordWindow(fSettings);
	fPasswordWindow->Run();

	// Create the tab view
	fTabView = new TabView();
	fTabView->SetBorder(B_NO_BORDER);

	// Create the controls inside the tabs
	fFadeView = new FadeView(B_TRANSLATE("General"), fSettings);
	fModulesView = new ModulesView(B_TRANSLATE("Screensavers"), fSettings);

	fTabView->AddTab(fFadeView);
	fTabView->AddTab(fModulesView);

	// Create the topmost background view
	BView* topView = new BView("topView", B_WILL_DRAW);
	topView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	topView->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
		B_ALIGN_USE_FULL_HEIGHT));
	topView->SetExplicitMinSize(BSize(fMinWidth, fMinHeight));
	BLayoutBuilder::Group<>(topView, B_VERTICAL)
		.SetInsets(0, B_USE_DEFAULT_SPACING, 0, B_USE_WINDOW_SPACING)
		.Add(fTabView)
		.End();

	SetLayout(new BGroupLayout(B_VERTICAL));
	GetLayout()->AddView(topView);

	fTabView->Select(fSettings.WindowTab());

	if (fSettings.WindowFrame().left > 0 && fSettings.WindowFrame().top > 0)
		MoveTo(fSettings.WindowFrame().left, fSettings.WindowFrame().top);

	if (fSettings.WindowFrame().Width() > 0
		&& fSettings.WindowFrame().Height() > 0) {
		ResizeTo(fSettings.WindowFrame().Width(),
			fSettings.WindowFrame().Height());
	}

	CenterOnScreen();
}


ScreenSaverWindow::~ScreenSaverWindow()
{
	Hide();
	fFadeView->UpdateStatus();
	fSettings.SetWindowTab(fTabView->Selection());

	delete fTabView->RemoveTab(1);
		// We delete this here in order to make sure the module view saves its
		// state while the window is still intact.

	fSettings.Save();
}


void
ScreenSaverWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgChangePassword:
			fPasswordWindow->CenterIn(Frame());
			fPasswordWindow->Show();
			break;

		case kMsgUpdateList:
			fModulesView->EmptyScreenSaverList();
			fModulesView->PopulateScreenSaverList();
			break;

		default:
			BWindow::MessageReceived(message);
	}
}


void
ScreenSaverWindow::ScreenChanged(BRect frame, color_space colorSpace)
{
	fFadeView->UpdateTurnOffScreen();
}


bool
ScreenSaverWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
