/*
 * Copyright 2003-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "MouseAreaView.h"
#include "PreviewView.h"
#include "ScreenSaverItem.h"
#include "ScreenSaverWindow.h"

#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <Directory.h>
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

#include <stdio.h>

#include <Debug.h>
#define CALLED() PRINT(("%s\n", __PRETTY_FUNCTION__))


const uint32 kPreviewMonitorGap = 16;
const uint32 kMinSettingsWidth = 230;
const uint32 kMinSettingsHeight = 120;

const int32 kMsgSaverSelected = 'SSEL';
const int32 kMsgTestSaver = 'TEST';
const int32 kMsgAddSaver = 'ADD ';
const int32 kMsgPasswordCheckBox = 'PWCB';
const int32 kMsgRunSliderChanged = 'RSCG';
const int32 kMsgPasswordSliderChanged = 'PWCG';
const int32 kMsgChangePassword = 'PWBT';
const int32 kMsgEnableScreenSaverBox = 'ESCH';

const int32 kMsgTurnOffCheckBox = 'TUOF';
const int32 kMsgTurnOffSliderChanged = 'TUCG';

static const uint32 kTimeSliderChanged = 'tsch';

class TimeSlider : public BSlider {
	public:
		TimeSlider(BRect frame, const char* name, uint32 modificationMessage);
		virtual ~TimeSlider();

		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage* message);
		virtual void SetValue(int32 value);

		void SetTime(bigtime_t useconds);
		bigtime_t Time();

	private:
		void _TimeToString(bigtime_t useconds, BString& string);

		uint32	fModificationMessage;
};

class FadeView : public BView {
	public:
		FadeView(BRect frame, const char* name, ScreenSaverPrefs& prefs);

		virtual void AttachedToWindow();
};

class ModulesView : public BView {
	public:
		ModulesView(BRect frame, const char* name, ScreenSaverPrefs& prefs);
		virtual ~ModulesView();

		virtual void DetachedFromWindow();
		virtual void AttachedToWindow();
		virtual void AllAttached();
		virtual void MessageReceived(BMessage* message);

		void PopulateScreenSaverList();
		void SaveState();

	private:
		static int _CompareScreenSaverItems(const void* left, const void* right);
		BScreenSaver* _ScreenSaver();
		void _CloseSaver();
		void _OpenSaver();

		BFilePanel*		fFilePanel;
		BListView*		fListView;
		BButton*		fTestButton;
		BButton*		fAddButton;

		ScreenSaverPrefs& fPrefs;
		ScreenSaverRunner* fSaverRunner;
		BString			fCurrentName;

		BBox*			fSettingsBox;
		BView*			fSettingsView;

		PreviewView*	fPreviewView;
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
static const int32 kTimeUnitCount = sizeof(kTimeInUnits) / sizeof(kTimeInUnits[0]);


TimeSlider::TimeSlider(BRect frame, const char* name, uint32 modificationMessage)
	: BSlider(frame, name, "0 minutes", new BMessage(kTimeSliderChanged),
		0, kTimeUnitCount - 1, B_TRIANGLE_THUMB, B_FOLLOW_LEFT_RIGHT),
	fModificationMessage(modificationMessage)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetModificationMessage(new BMessage(kTimeSliderChanged));
	SetBarThickness(10);
}


TimeSlider::~TimeSlider()
{
}


void
TimeSlider::AttachedToWindow()
{
	SetTarget(this);
}


void
TimeSlider::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kTimeSliderChanged:
			Window()->PostMessage(fModificationMessage);
			break;

		default:
			BSlider::MessageReceived(message);
	}
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
TimeSlider::Time()
{
	return 1000000LL * kTimeInUnits[Value()];
}


void
TimeSlider::_TimeToString(bigtime_t useconds, BString& string)
{
	useconds /= 1000000LL;
		// convert to seconds

	string = "";

	// hours
	uint32 hours = useconds / 3600;
	if (hours != 0)
		string << hours << " hours";

	useconds %= 3600;

	// minutes
	uint32 minutes = useconds / 60;
	if (hours != 0)
		string << " ";
	if (minutes != 0)
		string << minutes << " minutes";

	useconds %= 60;

	// seconds
	uint32 seconds = useconds / 60;
	if (hours != 0 || minutes != 0)
		string << " ";
	if (seconds != 0)
		string << seconds << " seconds";
}


//	#pragma mark -


FadeView::FadeView(BRect rect, const char* name, ScreenSaverPrefs& prefs)
	: BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW)
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


ModulesView::ModulesView(BRect rect, const char* name, ScreenSaverPrefs& prefs)
	: BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW),
	fPrefs(prefs),
	fSaverRunner(NULL),
	fSettingsView(NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fTestButton = new BButton(rect, "TestButton", "Test", new BMessage(kMsgTestSaver),
		B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	float width, height;
	fTestButton->GetPreferredSize(&width, &height);
	fTestButton->ResizeTo(width + 16, height);
	fTestButton->MoveTo(8, rect.bottom - 8 - height);
	AddChild(fTestButton);

	rect = fTestButton->Frame();
	rect.OffsetBy(fTestButton->Bounds().Width() + 8, 0);
	fAddButton = new BButton(rect, "AddButton", "Add" B_UTF8_ELLIPSIS,
		new BMessage(kMsgAddSaver), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
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
	AddChild(fSettingsBox = new BBox(rect, "SettingsBox", B_FOLLOW_ALL, B_WILL_DRAW));
	fSettingsBox->SetLabel("Module settings");

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

			ScreenSaverItem* item = (ScreenSaverItem *)fListView->ItemAt(selection);
			if (item == NULL)
				break;

			if (!strcmp(item->Text(), "Blackness"))
				fPrefs.SetModuleName("");
			else
				fPrefs.SetModuleName(item->Text());

			_CloseSaver();
			_OpenSaver();
			break;
		}

		case kMsgTestSaver:
			SaveState();
			fPrefs.SaveSettings();

			be_roster->Launch(SCREEN_BLANKER_SIG, &fPrefs.GetSettings());
			break;

		case kMsgAddSaver:
			fFilePanel->Show();
			break;

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
		fPrefs.SetState(fCurrentName.String(), &state);
}


void
ModulesView::PopulateScreenSaverList()
{
 	fListView->DeselectAll();
	while (ScreenSaverItem* item = (ScreenSaverItem *)fListView->RemoveItem((int32)0)) {
		delete item;
	}

	// Blackness is a built-in screen saver
	fListView->AddItem(new ScreenSaverItem("Blackness", ""));

	// Iterate over add-on directories, and add their files to the list view

	directory_which which[] = {B_BEOS_ADDONS_DIRECTORY, B_USER_ADDONS_DIRECTORY};
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
			
			if (!strcmp(fPrefs.ModuleName(), item->Text())
				|| (!strcmp(fPrefs.ModuleName(), "") 
					&& !strcmp(item->Text(), "Blackness")))
				selectItem = item;
		}
	}

	fListView->SortItems(_CompareScreenSaverItems);

	fListView->Select(fListView->IndexOf(selectItem));
	fListView->ScrollToSelection();
}


//! sorting function for ScreenSaverItems
int
ModulesView::_CompareScreenSaverItems(const void* left, const void* right) 
{
	ScreenSaverItem* leftItem  = *(ScreenSaverItem **)left;
	ScreenSaverItem* rightItem = *(ScreenSaverItem **)right;

	return strcmp(leftItem->Text(), rightItem->Text());
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

	if (saver != NULL)
		saver->StopConfig();

	SaveState();

	delete view;
	delete fSettingsView;
	delete fSaverRunner;

	fSettingsView = NULL;
	fSaverRunner = NULL;
}


void
ModulesView::_OpenSaver()
{
	// create new screen saver preview & config

   	BView* view = fPreviewView->AddPreview();
	fCurrentName = fPrefs.ModuleName();
	fSaverRunner = new ScreenSaverRunner(Window(), view, true, fPrefs);
	BScreenSaver* saver = _ScreenSaver();

	BRect rect = fSettingsBox->Bounds().InsetByCopy(4, 4);
#ifdef __HAIKU__
	rect.top += fSettingsBox->TopBorderOffset();
#else
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
		rect = BRect(15, 15, 20, 20);
		BStringView* stringView = new BStringView(rect, "module", fPrefs.ModuleName()[0]
			? fPrefs.ModuleName() : "Blackness");
		stringView->SetFont(be_bold_font);
		stringView->ResizeToPreferred();
		fSettingsView->AddChild(stringView);

		rect.OffsetBy(0, stringView->Bounds().Height() + 4);
		stringView = new BStringView(rect, "info", saver || !fPrefs.ModuleName()[0]
			? "No options available" : "Could not load screen saver");
		stringView->ResizeToPreferred();
		fSettingsView->AddChild(stringView);
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
		if ((view->ResizingMode() & _rule_(0, 0xf, 0, 0xf)) == B_FOLLOW_LEFT_RIGHT) {
			float width, height;
			view->GetPreferredSize(&width, &height);
			viewRight = view->Frame().left + width / 2;
		} else if ((view->ResizingMode() & _rule_(0, 0xf, 0, 0xf)) == B_FOLLOW_RIGHT)
			viewRight = 8 + view->Frame().Width();

		float viewBottom = view->Frame().bottom;
		if ((view->ResizingMode() & _rule_(0xf, 0, 0xf, 0)) == B_FOLLOW_TOP_BOTTOM) {
			float width, height;
			view->GetPreferredSize(&width, &height);
			viewBottom = view->Frame().top + height;
		} else if ((view->ResizingMode() & _rule_(0xf, 0, 0xf, 0)) == B_FOLLOW_BOTTOM)
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
	: BWindow(BRect(50, 50, 496, 375), "Screen Saver",
		B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS /*| B_NOT_ZOOMABLE | B_NOT_RESIZABLE*/),
	fFadeNowString(NULL),
	fFadeNowString2(NULL),
	fDontFadeString(NULL),
	fDontFadeString2(NULL),
	fFadeNow(NULL),
	fFadeNever(NULL)
{
	fPrefs.LoadSettings();

	BRect rect = Bounds();

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
	fModulesView = new ModulesView(rect, "Modules", fPrefs);

	fTabView->AddTab(fFadeView);
	fTabView->AddTab(fModulesView);
	background->AddChild(fTabView);

	// Create the password editing window
	fPasswordWindow = new PasswordWindow(fPrefs);
	fPasswordWindow->Run();

	// TODO: better limits!
	fMinWidth = 430;
	fMinHeight = 325;
	SetMinimalSizeLimit(fMinWidth, fMinHeight);
	MoveTo(fPrefs.WindowFrame().left, fPrefs.WindowFrame().top);
	ResizeTo(fPrefs.WindowFrame().Width(), fPrefs.WindowFrame().Height());

	fEnableCheckBox->SetValue(fPrefs.TimeFlags() & ENABLE_SAVER ? B_CONTROL_ON : B_CONTROL_OFF);
	fRunSlider->SetTime(fPrefs.BlankTime());
	fTurnOffSlider->SetTime(fPrefs.OffTime() + fPrefs.BlankTime());
	fFadeNow->SetCorner(fPrefs.GetBlankCorner());
	fFadeNever->SetCorner(fPrefs.GetNeverBlankCorner());
	fPasswordCheckBox->SetValue(fPrefs.LockEnable());
	fPasswordSlider->SetTime(fPrefs.PasswordTime());

	fTabView->Select(fPrefs.WindowTab());

	_UpdateTurnOffScreen();
	_UpdateStatus();
}


ScreenSaverWindow::~ScreenSaverWindow()
{
}


//! Create the controls for the fade tab
void
ScreenSaverWindow::_SetupFadeTab(BRect rect)
{
	fFadeView = new FadeView(rect, "Fade", fPrefs);

/*
	font_height fontHeight;
	be_plain_font->GetHeight(&fontHeight);
	float lineHeight = ceilf(fontHeight.ascent + fontHeight.descent);
*/
	fEnableCheckBox = new BCheckBox(BRect(0, 0, 1, 1), "EnableCheckBox",
		"Enable Screen Saver", new BMessage(kMsgEnableScreenSaverBox));
	fEnableCheckBox->ResizeToPreferred();

	rect.InsetBy(8, 8);
	BBox* box = new BBox(rect, "EnableScreenSaverBox", B_FOLLOW_ALL);
	box->SetLabel(fEnableCheckBox);
	fFadeView->AddChild(box);

	// Run Module
	rect.top = fEnableCheckBox->Bounds().bottom + 8.0f;
	BStringView* stringView = new BStringView(rect, NULL, "Run module");
	stringView->ResizeToPreferred();
	box->AddChild(stringView);

	float topEdge = rect.top;
	fRunSlider = new TimeSlider(BRect(135, topEdge, 420, topEdge + 20),
		"RunSlider", kMsgRunSliderChanged);
	float width, height;
	fRunSlider->GetPreferredSize(&width, &height);
	fRunSlider->ResizeTo(fRunSlider->Bounds().Width(), height);
	box->AddChild(fRunSlider);

	// Turn Off
	topEdge += fRunSlider->Bounds().Height() + 4.0f;
	fTurnOffCheckBox = new BCheckBox(BRect(20, topEdge-2, 127, topEdge + 20),
		"TurnOffScreenCheckBox", "Turn off screen", new BMessage(kMsgTurnOffCheckBox));
	fTurnOffCheckBox->ResizeToPreferred();
	box->AddChild(fTurnOffCheckBox);

	fTurnOffSlider = new TimeSlider(BRect(135, topEdge, 420, topEdge + 20),
		"TurnOffSlider", kMsgTurnOffSliderChanged);
	fTurnOffSlider->ResizeTo(fTurnOffSlider->Bounds().Width(), height);
	box->AddChild(fTurnOffSlider);

	// Password
	topEdge += fTurnOffSlider->Bounds().Height() + 4.0f;
	fPasswordCheckBox = new BCheckBox(BRect(20, topEdge - 2, 127, topEdge + 20),
		"PasswordCheckbox", "Password lock", new BMessage(kMsgPasswordCheckBox));
	fPasswordCheckBox->ResizeToPreferred();
	box->AddChild(fPasswordCheckBox);

	fPasswordSlider = new TimeSlider(BRect(135, topEdge, 420, topEdge + 20),
		"PasswordSlider", kMsgPasswordSliderChanged);
	fPasswordSlider->ResizeTo(fPasswordSlider->Bounds().Width(), height);
	box->AddChild(fPasswordSlider);

	topEdge += fPasswordSlider->Bounds().Height() + 4.0f;
	fPasswordButton = new BButton(BRect(390, topEdge, 410, topEdge + 24), "PasswordButton",
		"Password" B_UTF8_ELLIPSIS, new BMessage(kMsgChangePassword));
	fPasswordButton->ResizeToPreferred();
	fPasswordButton->MoveBy(-fPasswordButton->Bounds().Width(), 0);
	box->AddChild(fPasswordButton);

	// Bottom

	box->AddChild(fFadeNow = new MouseAreaView(BRect(20,205,80,260),"fadeNow"));
	box->AddChild(fFadeNever = new MouseAreaView(BRect(220,205,280,260),"fadeNever"));

	box->AddChild(fFadeNowString = new BStringView(BRect(90,215,193,230),"FadeNowString","Fade now when"));
	fFadeNowString->SetText("Fade now when");
	fFadeNowString->SetAlignment(B_ALIGN_LEFT);

	box->AddChild(fFadeNowString2 = new BStringView(BRect(90,233,193,248),"FadeNowString2","mouse is here"));
	fFadeNowString2->SetText("mouse is here");
	fFadeNowString2->SetAlignment(B_ALIGN_LEFT);

	box->AddChild(fDontFadeString = new BStringView(BRect(290,215,387,230),"DontFadeString","Don't fade when"));
	fDontFadeString->SetText("Don't fade when");
	fDontFadeString->SetAlignment(B_ALIGN_LEFT);

	box->AddChild(fDontFadeString2 = new BStringView(BRect(290,233,387,248),"DontFadeString2","mouse is here"));
	fDontFadeString2->SetText("mouse is here");
	fDontFadeString2->SetAlignment(B_ALIGN_LEFT);
}


void
ScreenSaverWindow::_UpdateTurnOffScreen()
{
	bool enabled = (fPrefs.TimeFlags() & ENABLE_DPMS_MASK) != 0;

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
	fPrefs.SetWindowFrame(Frame());	
	fPrefs.SetWindowTab(fTabView->Selection());
	fPrefs.SetTimeFlags((enabled ? ENABLE_SAVER : 0)
		| (fTurnOffCheckBox->Value() ? fTurnOffScreenFlags : 0));
	fPrefs.SetBlankTime(fRunSlider->Time());
	bigtime_t offTime = fTurnOffSlider->Time() - fPrefs.BlankTime();
	fPrefs.SetOffTime(offTime);
	fPrefs.SetSuspendTime(offTime);
	fPrefs.SetStandbyTime(offTime);
	fPrefs.SetBlankCorner(fFadeNow->Corner());
	fPrefs.SetNeverBlankCorner(fFadeNever->Corner());
	fPrefs.SetLockEnable(fPasswordCheckBox->Value());
	fPrefs.SetPasswordTime(fPasswordSlider->Time());

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
	switch (msg->what) {
		// "Fade" tab

		case kMsgRunSliderChanged:
			if (fRunSlider->Value() > fTurnOffSlider->Value())
				fTurnOffSlider->SetValue(fRunSlider->Value());

			if (fRunSlider->Value() > fPasswordSlider->Value())
				fPasswordSlider->SetValue(fRunSlider->Value());
			break;

		case kMsgTurnOffCheckBox:
			fTurnOffSlider->SetEnabled(fTurnOffCheckBox->Value() == B_CONTROL_ON);
			break;

		case kMsgTurnOffSliderChanged:
			if (fRunSlider->Value() > fTurnOffSlider->Value())
				fRunSlider->SetValue(fTurnOffSlider->Value());
			break;

		case kMsgPasswordSliderChanged:
			if (fPasswordSlider->Value() < fRunSlider->Value())
				fRunSlider->SetValue(fPasswordSlider->Value());
			break;

		case kMsgPasswordCheckBox:
		case kMsgEnableScreenSaverBox:
			_UpdateStatus();
			break;

		case kMsgChangePassword:
			fPasswordWindow->Show();
			break;

		// "Modules" tab

		case kMsgUpdateList:
			fModulesView->PopulateScreenSaverList();
			break;
  	}
	BWindow::MessageReceived(msg);
}


void
ScreenSaverWindow::ScreenChanged(BRect frame, color_space colorSpace)
{
	_UpdateTurnOffScreen();
}


bool
ScreenSaverWindow::QuitRequested()
{
	_UpdateStatus();
	fModulesView->SaveState();
	fPrefs.SaveSettings();

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}       

