/*
 * Copyright 2003-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "ScreenSaverWindow.h"

#include <stdio.h>

#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <Directory.h>
#include <DurationFormat.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Font.h>
#include <ListView.h>
#include <Path.h>
#include <Roster.h>
#include <Screen.h>
#include <ScreenSaver.h>
#include <ScrollView.h>
#include <Slider.h>
#include <StringView.h>
#include <TabView.h>
#include <TextView.h>

#include <BuildScreenSaverDefaultSettingsView.h>

#include "PreviewView.h"
#include "ScreenCornerSelector.h"
#include "ScreenSaverItem.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ScreenSaver"

const uint32 kPreviewMonitorGap = 16;
const uint32 kMinSettingsWidth = 230;
const uint32 kMinSettingsHeight = 120;

const int32 kMsgSaverSelected = 'SSEL';
const int32 kMsgTestSaver = 'TEST';
const int32 kMsgAddSaver = 'ADD ';
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


class TimeSlider : public BSlider {
public:
								TimeSlider(BRect frame, const char* name,
									uint32 changedMessage,
									uint32 updateMessage);
	virtual						~TimeSlider();

	virtual	void				AttachedToWindow();
	virtual	void				SetValue(int32 value);

			void				SetTime(bigtime_t useconds);
			bigtime_t			Time() const;

private:
			void				_TimeToString(bigtime_t useconds,
									BString& string);
};


class FadeView : public BView {
public:
								FadeView(BRect frame, const char* name,
									ScreenSaverSettings& settings);

	virtual	void				AttachedToWindow();
};


class ModulesView : public BView {
public:
								ModulesView(BRect frame, const char* name,
									ScreenSaverSettings& settings);
	virtual						~ModulesView();

	virtual	void				DetachedFromWindow();
	virtual	void				AttachedToWindow();
	virtual	void				AllAttached();
	virtual	void				MessageReceived(BMessage* message);

			void				PopulateScreenSaverList();
			void				SaveState();

private:
	static	int					_CompareScreenSaverItems(const void* left,
									const void* right);

			BScreenSaver*		_ScreenSaver();
			void				_CloseSaver();
			void				_OpenSaver();

private:
			BFilePanel*			fFilePanel;
			BListView*			fListView;
			BButton*			fTestButton;
			BButton*			fAddButton;

			ScreenSaverSettings& fSettings;
			ScreenSaverRunner*	fSaverRunner;
			BString				fCurrentName;

			BBox*				fSettingsBox;
			BView*				fSettingsView;

			PreviewView*		fPreviewView;

			team_id				fScreenSaverTestTeam;
};


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


TimeSlider::TimeSlider(BRect frame, const char* name, uint32 changedMessage,
		uint32 updateMessage)
	:
	BSlider(frame, name, B_TRANSLATE("30 seconds"),
		new BMessage(changedMessage),
		0, kTimeUnitCount - 1, B_TRIANGLE_THUMB, B_FOLLOW_LEFT_RIGHT)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetModificationMessage(new BMessage(updateMessage));
	SetBarThickness(10);
}


TimeSlider::~TimeSlider()
{
}


void
TimeSlider::AttachedToWindow()
{
	BSlider::AttachedToWindow();
	SetTarget(Window());
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
	formatter.Format(0, useconds, &string);
}


//	#pragma mark -


FadeView::FadeView(BRect rect, const char* name, ScreenSaverSettings& settings)
	:
	BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void
FadeView::AttachedToWindow()
{
	if (Parent() != NULL) {
		// We adopt the size of our parent view (in case the window
		// was resized during our absence (BTabView...)
		ResizeTo(Parent()->Bounds().Width(), Parent()->Bounds().Height());
	}
}


//	#pragma mark -


ModulesView::ModulesView(BRect rect, const char* name,
		ScreenSaverSettings& settings)
	:
	BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW),
	fSettings(settings),
	fSaverRunner(NULL),
	fSettingsView(NULL),
	fScreenSaverTestTeam(-1)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fTestButton = new BButton(rect, "TestButton", B_TRANSLATE("Test"),
		new BMessage(kMsgTestSaver), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	float width, height;
	fTestButton->GetPreferredSize(&width, &height);
	fTestButton->ResizeTo(width + 16, height);
	fTestButton->MoveTo(8, rect.bottom - 8 - height);
	AddChild(fTestButton);

	rect = fTestButton->Frame();
	rect.OffsetBy(fTestButton->Bounds().Width() + 8, 0);
	fAddButton = new BButton(rect, "AddButton",
		B_TRANSLATE("Add" B_UTF8_ELLIPSIS), new BMessage(kMsgAddSaver),
		B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(fAddButton);

	rect = Bounds().InsetByCopy(8 + kPreviewMonitorGap, 12);
	rect.right = fAddButton->Frame().right - kPreviewMonitorGap;
	rect.bottom = rect.top + 3 * rect.Width() / 4;
		// 4:3 monitor

	fPreviewView = new PreviewView(rect, "preview");
	AddChild(fPreviewView);

	rect.left = 8;
	rect.right -= B_V_SCROLL_BAR_WIDTH + 2 - kPreviewMonitorGap;
		// scroll view border
	rect.top = rect.bottom + 14;
	rect.bottom = fTestButton->Frame().top - 10;
	fListView = new BListView(rect, "SaversListView", B_SINGLE_SELECTION_LIST,
		B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM);
	fListView->SetSelectionMessage(new BMessage(kMsgSaverSelected));
	AddChild(new BScrollView("scroll_list", fListView,
		B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, 0, false, true));

	rect = Bounds().InsetByCopy(8, 8);
	rect.left = fAddButton->Frame().right + 8;
	AddChild(fSettingsBox = new BBox(rect, "SettingsBox", B_FOLLOW_ALL,
		B_WILL_DRAW));
	fSettingsBox->SetLabel(B_TRANSLATE("Screensaver settings"));

	PopulateScreenSaverList();
	fFilePanel = new BFilePanel();
}


ModulesView::~ModulesView()
{
	delete fFilePanel;
}


void
ModulesView::DetachedFromWindow()
{
	SaveState();
	_CloseSaver();
}


void
ModulesView::AttachedToWindow()
{
	if (Parent() != NULL) {
		// We adopt the size of our parent view (in case the window
		// was resized during our absence (BTabView...)
		ResizeTo(Parent()->Bounds().Width(), Parent()->Bounds().Height());
	}

	_OpenSaver();

	fListView->SetTarget(this);
	fTestButton->SetTarget(this);
	fAddButton->SetTarget(this);
}


void
ModulesView::AllAttached()
{
	// This only works after the view has been attached
	fListView->ScrollToSelection();
}


void
ModulesView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgSaverSelected:
		{
			int selection = fListView->CurrentSelection();
			if (selection < 0)
				break;

			ScreenSaverItem* item
				= (ScreenSaverItem*)fListView->ItemAt(selection);
			if (item == NULL)
				break;

			if (!strcmp(item->Text(), B_TRANSLATE("Blackness")))
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
			if (be_roster->Launch(SCREEN_BLANKER_SIG, &fSettings.Message(),
					&fScreenSaverTestTeam) == B_OK)
				break;

			// Try really hard to launch it. It's very likely that this fails
			// when we run from the CD, and there is only an incomplete mime
			// database for example...
			BPath path;
			if (find_directory(B_SYSTEM_BIN_DIRECTORY, &path) != B_OK
				|| path.Append("screen_blanker") != B_OK)
				path.SetTo("/bin/screen_blanker");

			BEntry entry(path.Path());
			entry_ref ref;
			if (entry.GetRef(&ref) == B_OK) {
				be_roster->Launch(&ref, &fSettings.Message(),
					&fScreenSaverTestTeam);
			}
			break;
		}
		case kMsgAddSaver:
			fFilePanel->Show();
			break;

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
	BScreenSaver* saver = _ScreenSaver();
	if (saver == NULL)
		return;

	BMessage state;
	if (saver->SaveState(&state) == B_OK)
		fSettings.SetModuleState(fCurrentName.String(), &state);
}


void
ModulesView::PopulateScreenSaverList()
{
 	fListView->DeselectAll();
	while (ScreenSaverItem* item
			= (ScreenSaverItem*)fListView->RemoveItem((int32)0)) {
		delete item;
	}

	// Blackness is a built-in screen saver
	fListView->AddItem(new ScreenSaverItem(B_TRANSLATE("Blackness"), ""));

	// Iterate over add-on directories, and add their files to the list view

	directory_which which[] = {
		B_USER_ADDONS_DIRECTORY,
		B_COMMON_ADDONS_DIRECTORY,
		B_SYSTEM_ADDONS_DIRECTORY,
	};
	ScreenSaverItem* selectItem = NULL;

	for (uint32 i = 0; i < sizeof(which) / sizeof(which[0]); i++) {
		BPath basePath;
		find_directory(which[i], &basePath);
		basePath.Append("Screen Savers", true);

		BDirectory dir(basePath.Path());
		BEntry entry;
		while (dir.GetNextEntry(&entry, true) == B_OK) {
			char name[B_FILE_NAME_LENGTH];
			if (entry.GetName(name) != B_OK)
				continue;

			BPath path = basePath;
			path.Append(name);

			ScreenSaverItem* item = new ScreenSaverItem(name, path.Path());
			fListView->AddItem(item);

			if (!strcmp(fSettings.ModuleName(), item->Text())
				|| (!strcmp(fSettings.ModuleName(), "")
					&& !strcmp(item->Text(), B_TRANSLATE("Blackness"))))
				selectItem = item;
		}
	}

	fListView->SortItems(_CompareScreenSaverItems);

	fListView->Select(fListView->IndexOf(selectItem));
	fListView->ScrollToSelection();
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
ModulesView::_ScreenSaver()
{
	if (fSaverRunner != NULL)
		return fSaverRunner->ScreenSaver();

	return NULL;
}


void
ModulesView::_CloseSaver()
{
	// remove old screen saver preview & config

	BScreenSaver* saver = _ScreenSaver();
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
	fSaverRunner = new ScreenSaverRunner(Window(), view, true, fSettings);
	BScreenSaver* saver = _ScreenSaver();

#ifdef __HAIKU__
	BRect rect = fSettingsBox->InnerFrame().InsetByCopy(4, 4);
#else
	BRect rect = fSettingsBox->Bounds().InsetByCopy(4, 4);
	rect.top += 14;
#endif
	fSettingsView = new BView(rect, "SettingsView", B_FOLLOW_ALL, B_WILL_DRAW);
	fSettingsView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fSettingsBox->AddChild(fSettingsView);

	if (saver != NULL) {
		fSaverRunner->Run();
		saver->StartConfig(fSettingsView);
	}

	if (fSettingsView->ChildAt(0) == NULL) {
		// There are no settings at all, we add the module name here to
		// let it look a bit better at least.
		BPrivate::BuildScreenSaverDefaultSettingsView(fSettingsView,
			fSettings.ModuleName()[0] ? fSettings.ModuleName() :
			B_TRANSLATE("Blackness"), saver || !fSettings.ModuleName()[0]
				? B_TRANSLATE("No options available") :
				B_TRANSLATE("Could not load screen saver"));
	}

	ScreenSaverWindow* window = dynamic_cast<ScreenSaverWindow*>(Window());
	if (window == NULL)
		return;

	// find the minimal size of the settings view

	float right = 0, bottom = 0;
	int32 i = 0;
	while ((view = fSettingsView->ChildAt(i++)) != NULL) {
		// very simple heuristic...
		float viewRight = view->Frame().right;
		if ((view->ResizingMode() & _rule_(0, 0xf, 0, 0xf))
				== B_FOLLOW_LEFT_RIGHT) {
			float width, height;
			view->GetPreferredSize(&width, &height);
			viewRight = view->Frame().left + width / 2;
		} else if ((view->ResizingMode() & _rule_(0, 0xf, 0, 0xf))
				== B_FOLLOW_RIGHT)
			viewRight = 8 + view->Frame().Width();

		float viewBottom = view->Frame().bottom;
		if ((view->ResizingMode() & _rule_(0xf, 0, 0xf, 0))
				== B_FOLLOW_TOP_BOTTOM) {
			float width, height;
			view->GetPreferredSize(&width, &height);
			viewBottom = view->Frame().top + height;
		} else if ((view->ResizingMode() & _rule_(0xf, 0, 0xf, 0))
				== B_FOLLOW_BOTTOM)
			viewBottom = 8 + view->Frame().Height();

		if (viewRight > right)
			right = viewRight;
		if (viewBottom > bottom)
			bottom = viewBottom;
	}

	if (right < kMinSettingsWidth)
		right = kMinSettingsWidth;
	if (bottom < kMinSettingsHeight)
		bottom = kMinSettingsHeight;

	BPoint leftTop = fSettingsView->LeftTop();
	fSettingsView->ConvertToScreen(&leftTop);
	window->ConvertFromScreen(&leftTop);
	window->SetMinimalSizeLimit(leftTop.x + right + 16,
		leftTop.y + bottom + 16);
}


//	#pragma mark -


ScreenSaverWindow::ScreenSaverWindow()
	:
	BWindow(BRect(50, 50, 496, 375), B_TRANSLATE_SYSTEM_NAME("ScreenSaver"),
		B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	fSettings.Load();

	BRect rect = Bounds();
	fMinWidth = 430;
	fMinHeight = 325;

	// Create a background view
	BView *background = new BView(rect, "background", B_FOLLOW_ALL, 0);
	background->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(background);

	// Add the tab view to the background
	rect.top += 4;
	fTabView = new BTabView(rect, "tab_view");

	// Create the controls inside the tabs
	rect = fTabView->ContainerView()->Bounds();
	_SetupFadeTab(rect);
	fModulesView = new ModulesView(rect, B_TRANSLATE("Screensavers"), fSettings);

	fTabView->AddTab(fFadeView);
	fTabView->AddTab(fModulesView);
	background->AddChild(fTabView);

	// Create the password editing window
	fPasswordWindow = new PasswordWindow(fSettings);
	fPasswordWindow->Run();

	SetMinimalSizeLimit(fMinWidth, fMinHeight);
	MoveTo(fSettings.WindowFrame().left, fSettings.WindowFrame().top);
	ResizeTo(fSettings.WindowFrame().Width(), fSettings.WindowFrame().Height());

	fEnableCheckBox->SetValue(
		fSettings.TimeFlags() & ENABLE_SAVER ? B_CONTROL_ON : B_CONTROL_OFF);
	fRunSlider->SetTime(fSettings.BlankTime());
	fTurnOffSlider->SetTime(fSettings.OffTime() + fSettings.BlankTime());
	fFadeNow->SetCorner(fSettings.BlankCorner());
	fFadeNever->SetCorner(fSettings.NeverBlankCorner());
	fPasswordCheckBox->SetValue(fSettings.LockEnable());
	fPasswordSlider->SetTime(fSettings.PasswordTime());

	fTabView->Select(fSettings.WindowTab());

	_UpdateTurnOffScreen();
	_UpdateStatus();
}


ScreenSaverWindow::~ScreenSaverWindow()
{
	Hide();
	_UpdateStatus();

	delete fTabView->RemoveTab(1);
		// We delete this here in order to make sure the module view saves its
		// state while the window is still intact.

	fSettings.Save();
}


//! Create the controls for the "General" tab
void
ScreenSaverWindow::_SetupFadeTab(BRect rect)
{
	fFadeView = new FadeView(rect, B_TRANSLATE("General"), fSettings);

	float StringWidth1 = be_plain_font->StringWidth(B_TRANSLATE
		("Start screensaver"));
	float StringWidth2 = be_plain_font->StringWidth(B_TRANSLATE
		("Turn off screen"));
	float StringWidth3 = be_plain_font->StringWidth(B_TRANSLATE
		("Password lock"));
	
	float labelWidth = StringWidth1;
	if (labelWidth < StringWidth2)
		labelWidth = StringWidth2;
	if (labelWidth < StringWidth3)
		labelWidth = StringWidth3;

	labelWidth += 20.0f;

	font_height fontHeight;
	be_plain_font->GetHeight(&fontHeight);
	float textHeight = ceilf(fontHeight.ascent + fontHeight.descent);

	// taken from BRadioButton:
	float radioButtonOffset = 2 * floorf(textHeight / 2 - 2)
		+ floorf(textHeight / 2);

	fEnableCheckBox = new BCheckBox(BRect(0, 0, 1, 1), "EnableCheckBox",
		B_TRANSLATE("Enable screensaver"),
		new BMessage(kMsgEnableScreenSaverBox));
	fEnableCheckBox->ResizeToPreferred();

	rect.InsetBy(8, 8);
	BBox* box = new BBox(rect, "EnableScreenSaverBox", B_FOLLOW_ALL);
	box->SetLabel(fEnableCheckBox);
	fFadeView->AddChild(box);

	// Start Screensaver
	rect.left += radioButtonOffset + 6;
	rect.top = fEnableCheckBox->Bounds().bottom + 8.0f;
	rect.right = box->Bounds().right - 8;
	BStringView* stringView = new BStringView(rect, NULL,
		B_TRANSLATE("Start screensaver"));
	stringView->ResizeToPreferred();
	box->AddChild(stringView);

	rect.left += labelWidth - 4;
	fRunSlider = new TimeSlider(rect, "RunSlider", kMsgRunSliderChanged,
		kMsgRunSliderUpdate);
	float width, height;
	fRunSlider->GetPreferredSize(&width, &height);
	fRunSlider->ResizeTo(fRunSlider->Bounds().Width(), height);
	box->AddChild(fRunSlider);

	// Turn Off
	rect.left = 10;
	rect.OffsetBy(0, fRunSlider->Bounds().Height() + 4.0f);
	fTurnOffCheckBox = new BCheckBox(rect, "TurnOffScreenCheckBox",
		B_TRANSLATE("Turn off screen"), new BMessage(kMsgTurnOffCheckBox));
	fTurnOffCheckBox->ResizeToPreferred();
	box->AddChild(fTurnOffCheckBox);

	rect.top += 3;
	rect.left += radioButtonOffset + labelWidth;
	fTurnOffSlider = new TimeSlider(rect, "TurnOffSlider",
		kMsgTurnOffSliderChanged, kMsgTurnOffSliderUpdate);
	fTurnOffSlider->ResizeTo(fTurnOffSlider->Bounds().Width(), height);
	box->AddChild(fTurnOffSlider);

	rgb_color textColor = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_DISABLED_LABEL_TINT);
	BRect textRect(0, 0, fTurnOffSlider->Bounds().Width(), height);
	textRect.InsetBy(0, 3);
	fTurnOffNotSupported = new BTextView(rect, "not_supported",
		textRect, be_plain_font, &textColor, B_FOLLOW_ALL, B_WILL_DRAW);
	fTurnOffNotSupported->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fTurnOffNotSupported->MakeEditable(false);
	fTurnOffNotSupported->MakeSelectable(false);
	fTurnOffNotSupported->SetText(
		B_TRANSLATE("Display Power Management Signaling not available"));

	fTurnOffNotSupported->ResizeTo(fTurnOffSlider->Bounds().Width(), height);
	box->AddChild(fTurnOffNotSupported);

	// Password
	rect.left = 10;
	rect.OffsetBy(0, fTurnOffSlider->Bounds().Height() + 4.0f);
	fPasswordCheckBox = new BCheckBox(rect, "PasswordCheckbox",
		B_TRANSLATE("Password lock"), new BMessage(kMsgPasswordCheckBox));
	fPasswordCheckBox->ResizeToPreferred();
	box->AddChild(fPasswordCheckBox);

	rect.top += 3;
	rect.left += radioButtonOffset + labelWidth;
	fPasswordSlider = new TimeSlider(rect, "PasswordSlider",
		kMsgPasswordSliderChanged, kMsgPasswordSliderUpdate);
	fPasswordSlider->ResizeTo(fPasswordSlider->Bounds().Width(), height);
	box->AddChild(fPasswordSlider);

	rect.OffsetBy(0, fTurnOffSlider->Bounds().Height() + 4.0f);
	rect.left = rect.right;
	fPasswordButton = new BButton(rect, "PasswordButton",
		B_TRANSLATE("Password" B_UTF8_ELLIPSIS),
		new BMessage(kMsgChangePassword), B_FOLLOW_TOP | B_FOLLOW_RIGHT);
	fPasswordButton->ResizeToPreferred();
	fPasswordButton->MoveBy(-fPasswordButton->Bounds().Width(), 0);
	box->AddChild(fPasswordButton);

	// Bottom

	float monitorHeight = 10 + textHeight * 3;
	float monitorWidth = monitorHeight * 4 / 3;
	rect.left = 11;
	rect.top = box->Bounds().Height() - 15 - monitorHeight;
	rect.right = rect.left + monitorWidth;
	rect.bottom = rect.top + monitorHeight;
	box->AddChild(fFadeNow = new ScreenCornerSelector(rect, "FadeNow",
		new BMessage(kMsgFadeCornerChanged), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM));

	rect.OffsetBy(monitorWidth + 10, 0);
	stringView = new BStringView(rect, NULL, B_TRANSLATE("Fade now when"),
		B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	stringView->ResizeToPreferred();
	float maxWidth = stringView->Bounds().Width();
	box->AddChild(stringView);

	rect.OffsetBy(0, stringView->Bounds().Height());
	stringView = new BStringView(rect, NULL, B_TRANSLATE("mouse is here"),
		B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	stringView->ResizeToPreferred();
	if (maxWidth < stringView->Bounds().Width())
		maxWidth = stringView->Bounds().Width();
	box->AddChild(stringView);

	rect.left += maxWidth + 20;
	rect.top = box->Bounds().Height() - 15 - monitorHeight;
	rect.right = rect.left + monitorWidth;
	rect.bottom = rect.top + monitorHeight;
	box->AddChild(fFadeNever = new ScreenCornerSelector(rect, "FadeNever",
		new BMessage(kMsgNeverFadeCornerChanged),
		B_FOLLOW_LEFT | B_FOLLOW_BOTTOM));

	rect.OffsetBy(monitorWidth + 10, 0);
	stringView = new BStringView(rect, NULL,B_TRANSLATE("Don't fade when"),
		B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	stringView->ResizeToPreferred();
	if (maxWidth < stringView->Bounds().Width())
		maxWidth = stringView->Bounds().Width();
	box->AddChild(stringView);

	rect.OffsetBy(0, stringView->Bounds().Height());
	stringView = new BStringView(rect, NULL, B_TRANSLATE("mouse is here"),
		B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	stringView->ResizeToPreferred();
	if (maxWidth < stringView->Bounds().Width())
		maxWidth = stringView->Bounds().Width();
	box->AddChild(stringView);

	float size = rect.left + maxWidth + 40;
	if (fMinWidth < size)
		fMinWidth = size;
	size = fPasswordButton->Frame().bottom + box->Frame().top
		+ monitorHeight + 40 + textHeight * 2;
	if (fMinHeight < size)
		fMinHeight = size;
}


void
ScreenSaverWindow::_UpdateTurnOffScreen()
{
	bool enabled = (fSettings.TimeFlags() & ENABLE_DPMS_MASK) != 0;

	BScreen screen(this);
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
ScreenSaverWindow::_UpdateStatus()
{
	DisableUpdates();

	bool enabled = fEnableCheckBox->Value() == B_CONTROL_ON;
	fPasswordCheckBox->SetEnabled(enabled);
	fTurnOffCheckBox->SetEnabled(enabled && fTurnOffScreenFlags != 0);
	fRunSlider->SetEnabled(enabled);
	fTurnOffSlider->SetEnabled(enabled && fTurnOffCheckBox->Value());
	fPasswordSlider->SetEnabled(enabled && fPasswordCheckBox->Value());
	fPasswordButton->SetEnabled(enabled && fPasswordCheckBox->Value());

	EnableUpdates();

	// Update the saved preferences
	fSettings.SetWindowFrame(Frame());
	fSettings.SetWindowTab(fTabView->Selection());
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
ScreenSaverWindow::SetMinimalSizeLimit(float width, float height)
{
	if (width < fMinWidth)
		width = fMinWidth;
	if (height < fMinHeight)
		height = fMinHeight;

	SetSizeLimits(width, 32767, height, 32767);
}


void
ScreenSaverWindow::MessageReceived(BMessage *msg)
{
	// "Settings" tab, slider updates

	switch (msg->what) {
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
	}

	switch (msg->what) {
		// "General" tab

		case kMsgTurnOffCheckBox:
			fTurnOffSlider->SetEnabled(
				fTurnOffCheckBox->Value() == B_CONTROL_ON);
			break;

		case kMsgRunSliderChanged:
		case kMsgTurnOffSliderChanged:
		case kMsgPasswordSliderChanged:
		case kMsgPasswordCheckBox:
		case kMsgEnableScreenSaverBox:
		case kMsgFadeCornerChanged:
		case kMsgNeverFadeCornerChanged:
			_UpdateStatus();
			fSettings.Save();
			break;

		case kMsgChangePassword:
			fPasswordWindow->Show();
			break;

		// "Screensavers" tab

		case kMsgUpdateList:
			fModulesView->PopulateScreenSaverList();
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


void
ScreenSaverWindow::ScreenChanged(BRect frame, color_space colorSpace)
{
	_UpdateTurnOffScreen();
}


bool
ScreenSaverWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

