/*
 * Copyright 2001-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Rafael Romo
 *		Stefano Ceccherini (burton666@libero.it)
 *		Andrew Bachmann
 *		Thomas Kurschel
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "AlertWindow.h"
#include "Constants.h"
#include "RefreshWindow.h"
#include "MonitorView.h"
#include "ScreenSettings.h"
#include "ScreenWindow.h"
#include "Utility.h"

/* Note, this headers defines a *private* interface to the Radeon accelerant.
 * It's a solution that works with the current BeOS interface that Haiku
 * adopted.
 * However, it's not a nice and clean solution. Don't use this header in any
 * application if you can avoid it. No other driver is using this, or should
 * be using this.
 * It will be replaced as soon as we introduce an updated accelerant interface
 * which may even happen before R1 hits the streets.
 */
#include "multimon.h"	// the usual: DANGER WILL, ROBINSON!

#include <Alert.h>
#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <InterfaceDefs.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Messenger.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <String.h>
#include <Roster.h>
#include <Window.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


const char* kBackgroundsSignature = "application/x-vnd.haiku-backgrounds";

// list of officially supported colour spaces
static const struct {
	color_space	space;
	int32		bits_per_pixel;
	const char*	label;
} kColorSpaces[] = {
	{ B_CMAP8, 8, "8 Bits/Pixel, 256 Colors" },
	{ B_RGB15, 15, "15 Bits/Pixel, 32768 Colors" },
	{ B_RGB16, 16, "16 Bits/Pixel, 65536 Colors" },
	{ B_RGB32, 32, "32 Bits/Pixel, 16 Million Colors" }
};
static const int32 kColorSpaceCount = sizeof(kColorSpaces) / sizeof(kColorSpaces[0]);

// list of standard refresh rates
static const int32 kRefreshRates[] = { 60, 70, 72, 75, 80, 85, 95, 100 };
static const int32 kRefreshRateCount = sizeof(kRefreshRates) / sizeof(kRefreshRates[0]);

// list of combine modes
static const struct {
	combine_mode	mode;
	const char		*name;
} kCombineModes[] = {
	{ kCombineDisable, "disable" },
	{ kCombineHorizontally, "horizontally" },
	{ kCombineVertically, "vertically" }
};
static const int32 kCombineModeCount = sizeof(kCombineModes) / sizeof(kCombineModes[0]);

enum {
	SHOW_COMBINE_FIELD		= 0x01,
	SHOW_SWAP_FIELD			= 0x02,
	SHOW_LAPTOP_PANEL_FIELD	= 0x04,
	SHOW_TV_STANDARD_FIELD	= 0x08,
};


static BString
tv_standard_to_string(uint32 mode)
{
	switch (mode) {
		case 0:		return "disabled";
		case 1:		return "NTSC";
		case 2:		return "NTSC Japan";
		case 3:		return "PAL BDGHI";
		case 4:		return "PAL M";
		case 5:		return "PAL N";
		case 6:		return "SECAM";
		case 101:	return "NTSC 443";
		case 102:	return "PAL 60";
		case 103:	return "PAL NC";
		default:
		{
			BString name;
			name << "??? (" << mode << ")";

			return name;
		}
	}
}


static void
resolution_to_string(screen_mode& mode, BString &string)
{
	string << mode.width << " x " << mode.height;
}


static void
refresh_rate_to_string(float refresh, BString &string,
	bool appendUnit = true, bool alwaysWithFraction = false)
{
	snprintf(string.LockBuffer(32), 32, "%.*g", refresh >= 100.0 ? 4 : 3, refresh);
	string.UnlockBuffer();

	if (appendUnit)
		string << " Hz";
}


static const char*
screen_errors(status_t status)
{
	switch (status) {
		case B_ENTRY_NOT_FOUND:
			return "Unknown Mode";
		// TODO: add more?

		default:
			return strerror(status);
	}
}


static float
max_label_width(BMenuField* control, float widestLabel)
{
	float labelWidth = control->StringWidth(control->Label());
	if (widestLabel < labelWidth)
		return labelWidth;
	return widestLabel;
}


static BRect
stack_and_align_menu_fields(const BList& menuFields)
{
	float widestLabel = 0.0;
	int32 count = menuFields.CountItems();
	for (int32 i = 0; i < count; i++) {
		BMenuField* menuField = (BMenuField*)menuFields.ItemAtFast(i);
		widestLabel = max_label_width(menuField, widestLabel);
	}

	// add some room (but only if there is text at all)
	if (widestLabel > 0.0)
		widestLabel += 5.0;

	// make all controls the same width
	float widestField = 0.0;
	for (int32 i = 0; i < count; i++) {
		BMenuField* menuField = (BMenuField*)menuFields.ItemAtFast(i);
		menuField->SetAlignment(B_ALIGN_RIGHT);
		menuField->SetDivider(widestLabel);
		menuField->ResizeToPreferred();
		widestField = max_c(menuField->Bounds().Width(), widestField);
	}

	// layout controls under each other, resize all to size
	// of largest of them (they could still have different
	// heights though)
	BMenuField* topMenuField = (BMenuField*)menuFields.FirstItem();
	BPoint leftTop = topMenuField->Frame().LeftTop();
	BRect frame = topMenuField->Frame();

	for (int32 i = 0; i < count; i++) {
		BMenuField* menuField = (BMenuField*)menuFields.ItemAtFast(i);
		menuField->MoveTo(leftTop);
		float height = menuField->Bounds().Height();
		menuField->ResizeTo(widestField, height);
		frame = frame | menuField->Frame();
		leftTop.y += height + 5.0;
	}

	return frame;
}


//	#pragma mark -


ScreenWindow::ScreenWindow(ScreenSettings *settings)
	: BWindow(settings->WindowFrame(), "Screen", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE, B_ALL_WORKSPACES),
	fIsVesa(false),
	fVesaApplied(false),
	fScreenMode(this),
	fChangingAllWorkspaces(false)
{
	BScreen screen(this);

	accelerant_device_info info;
	if (screen.GetDeviceInfo(&info) == B_OK && !strcasecmp(info.chipset, "VESA"))
		fIsVesa = true;

	fScreenMode.Get(fOriginal);
	fActive = fSelected = fOriginal;

	BView *view = new BView(Bounds(), "ScreenView", B_FOLLOW_ALL, B_WILL_DRAW);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);

	fSettings = settings;

	// we need the "Current Workspace" first to get its height

	BPopUpMenu *popUpMenu = new BPopUpMenu("Current Workspace", true, true);
	fAllWorkspacesItem = new BMenuItem("All Workspaces", new BMessage(WORKSPACE_CHECK_MSG));
	popUpMenu->AddItem(fAllWorkspacesItem);
	BMenuItem *item = new BMenuItem("Current Workspace", new BMessage(WORKSPACE_CHECK_MSG));
	if (_IsVesa()) {
		fAllWorkspacesItem->SetMarked(true);
		item->SetEnabled(false);
	} else
		item->SetMarked(true);
	popUpMenu->AddItem(item);

	BMenuField* workspaceMenuField = new BMenuField(BRect(0, 0, 100, 15),
		"WorkspaceMenu", NULL, popUpMenu, true);
	workspaceMenuField->ResizeToPreferred();

	// box on the left with workspace count and monitor view

	popUpMenu = new BPopUpMenu("", true, true);
	fWorkspaceCountField = new BMenuField(BRect(0.0, 0.0, 50.0, 15.0),
		"WorkspaceCountMenu", "Workspace count:", popUpMenu, true);
	float labelWidth = fWorkspaceCountField->StringWidth(fWorkspaceCountField->Label()) + 5.0;
	fWorkspaceCountField->SetDivider(labelWidth);

	fScreenBox = new BBox(BRect(0.0, 0.0, 100.0, 100.0), "left box");
	fScreenBox->AddChild(fWorkspaceCountField);

	for (int32 count = 1; count <= 32; count++) {
		BString workspaceCount;
		workspaceCount << count;

		BMessage *message = new BMessage(POP_WORKSPACE_CHANGED_MSG);
		message->AddInt32("workspace count", count);

		popUpMenu->AddItem(new BMenuItem(workspaceCount.String(),
			message));
	}

	item = popUpMenu->ItemAt(count_workspaces() - 1);
	if (item != NULL)
		item->SetMarked(true);

	fMonitorView = new MonitorView(BRect(0.0, 0.0, 80.0, 80.0), "monitor",
		screen.Frame().Width() + 1, screen.Frame().Height() + 1);
	fScreenBox->AddChild(fMonitorView);

	view->AddChild(fScreenBox);

	// box on the right with screen resolution, etc.

	fControlsBox = new BBox(BRect(0.0, 0.0, 100.0, 100.0));
	fControlsBox->SetLabel(workspaceMenuField);

	fResolutionMenu = new BPopUpMenu("resolution", true, true);

	uint16 previousWidth = 0, previousHeight = 0;
	for (int32 i = 0; i < fScreenMode.CountModes(); i++) {
		screen_mode mode = fScreenMode.ModeAt(i);

		if (mode.width == previousWidth && mode.height == previousHeight)
			continue;

		previousWidth = mode.width;
		previousHeight = mode.height;

		BMessage *message = new BMessage(POP_RESOLUTION_MSG);
		message->AddInt32("width", mode.width);
		message->AddInt32("height", mode.height);

		BString name;
		name << mode.width << " x " << mode.height;

		fResolutionMenu->AddItem(new BMenuItem(name.String(), message));
	}

	BRect rect(0.0, 0.0, 200.0, 15.0);
	// fResolutionField needs to be at the correct
	// left-top offset, because all other menu fields
	// will be layouted relative to it
	fResolutionField = new BMenuField(rect.OffsetToCopy(10.0, 30.0),
		"ResolutionMenu", "Resolution:", fResolutionMenu, true);
	fControlsBox->AddChild(fResolutionField);

	fColorsMenu = new BPopUpMenu("colors", true, true);

	for (int32 i = 0; i < kColorSpaceCount; i++) {
		BMessage *message = new BMessage(POP_COLORS_MSG);
		message->AddInt32("bits_per_pixel", kColorSpaces[i].bits_per_pixel);
		message->AddInt32("space", kColorSpaces[i].space);

		fColorsMenu->AddItem(new BMenuItem(kColorSpaces[i].label, message));
	}

	rect.OffsetTo(B_ORIGIN);

	fColorsField = new BMenuField(rect, "ColorsMenu", "Colors:", fColorsMenu, true);
	fControlsBox->AddChild(fColorsField);

	fRefreshMenu = new BPopUpMenu("refresh rate", true, true);

	BMessage *message;

	float min, max;
	if (fScreenMode.GetRefreshLimits(fActive, min, max) && min == max) {
		// This is a special case for drivers that only support a single
		// frequency, like the VESA driver
		BString name;
		name << min << " Hz";

		message = new BMessage(POP_REFRESH_MSG);
		message->AddFloat("refresh", min);

		fRefreshMenu->AddItem(item = new BMenuItem(name.String(), message));
		item->SetEnabled(false);
	} else {
		for (int32 i = 0; i < kRefreshRateCount; ++i) {
			BString name;
			name << kRefreshRates[i] << " Hz";

			message = new BMessage(POP_REFRESH_MSG);
			message->AddFloat("refresh", kRefreshRates[i]);

			fRefreshMenu->AddItem(new BMenuItem(name.String(), message));
		}

		message = new BMessage(POP_OTHER_REFRESH_MSG);

		fOtherRefresh = new BMenuItem("Other" B_UTF8_ELLIPSIS, message);
		fRefreshMenu->AddItem(fOtherRefresh);
	}

	fRefreshField = new BMenuField(rect, "RefreshMenu", "Refresh Rate:", fRefreshMenu, true);
	if (_IsVesa())
		fRefreshField->Hide();
	fControlsBox->AddChild(fRefreshField);

	view->AddChild(fControlsBox);

	uint32 controlsFlags = 0;

	// enlarged area for multi-monitor settings
	{
		bool dummy;
		uint32 dummy32;
		bool multiMonSupport;
		bool useLaptopPanelSupport;
		bool tvStandardSupport;

		multiMonSupport = TestMultiMonSupport(&screen) == B_OK;
		useLaptopPanelSupport = GetUseLaptopPanel(&screen, &dummy) == B_OK;
		tvStandardSupport = GetTVStandard(&screen, &dummy32) == B_OK;
		if (multiMonSupport) {
			controlsFlags |= SHOW_COMBINE_FIELD;
			controlsFlags |= SHOW_SWAP_FIELD;
		}
		if (useLaptopPanelSupport)
			controlsFlags |= SHOW_LAPTOP_PANEL_FIELD;

		// even if there is no support, we still create all controls
		// to make sure we don't access NULL pointers later on

		fCombineMenu = new BPopUpMenu("CombineDisplays", true, true);

		for (int32 i = 0; i < kCombineModeCount; i++) {
			message = new BMessage(POP_COMBINE_DISPLAYS_MSG);
			message->AddInt32("mode", kCombineModes[i].mode);

			fCombineMenu->AddItem(new BMenuItem(kCombineModes[i].name, message));
		}

		fCombineField = new BMenuField(rect, "CombineMenu",
			"Combine Displays:", fCombineMenu, true);
		fControlsBox->AddChild(fCombineField);

		if (!multiMonSupport)
			fCombineField->Hide();

		fSwapDisplaysMenu = new BPopUpMenu("SwapDisplays", true, true);

		// !order is important - we rely that boolean value == idx
		message = new BMessage(POP_SWAP_DISPLAYS_MSG);
		message->AddBool("swap", false);
		fSwapDisplaysMenu->AddItem(new BMenuItem("no", message));

		message = new BMessage(POP_SWAP_DISPLAYS_MSG);
		message->AddBool("swap", true);
		fSwapDisplaysMenu->AddItem(new BMenuItem("yes", message));

		fSwapDisplaysField = new BMenuField(rect, "SwapMenu", "Swap Displays:",
			fSwapDisplaysMenu, true);

		fControlsBox->AddChild(fSwapDisplaysField);
		if (!multiMonSupport)
			fSwapDisplaysField->Hide();

		fUseLaptopPanelMenu = new BPopUpMenu("UseLaptopPanel", true, true);

		// !order is important - we rely that boolean value == idx			
		message = new BMessage(POP_USE_LAPTOP_PANEL_MSG);
		message->AddBool("use", false);
		fUseLaptopPanelMenu->AddItem(new BMenuItem("if needed", message));

		message = new BMessage(POP_USE_LAPTOP_PANEL_MSG);
		message->AddBool("use", true);
		fUseLaptopPanelMenu->AddItem(new BMenuItem("always", message));

		fUseLaptopPanelField = new BMenuField(rect, "UseLaptopPanel", "Use Laptop Panel:",
			fUseLaptopPanelMenu, true);

		fControlsBox->AddChild(fUseLaptopPanelField);
		if (!useLaptopPanelSupport)
			fUseLaptopPanelField->Hide();

		fTVStandardMenu = new BPopUpMenu("TVStandard", true, true);

		// arbitrary limit
		uint32 i;
		for (i = 0; i < 100; ++i) {
			uint32 mode;

			if (GetNthSupportedTVStandard(&screen, i, &mode) != B_OK)
				break;

			BString name = tv_standard_to_string(mode);

			message = new BMessage(POP_TV_STANDARD_MSG);
			message->AddInt32("tv_standard", mode);

			fTVStandardMenu->AddItem(new BMenuItem(name.String(), message));
		}

		fTVStandardField = new BMenuField(rect, "tv standard", "Video Format:",
			fTVStandardMenu, true);
		fTVStandardField->SetAlignment(B_ALIGN_RIGHT);

		if (!tvStandardSupport || i == 0) {
			fTVStandardField->Hide();
		} else {
			controlsFlags |= SHOW_TV_STANDARD_FIELD;
		}

		fControlsBox->AddChild(fTVStandardField);
	}

	BRect buttonRect(0.0, 0.0, 30.0, 10.0);
	fBackgroundsButton = new BButton(buttonRect, "BackgroundsButton",
		"Set Background"B_UTF8_ELLIPSIS, new BMessage(BUTTON_LAUNCH_BACKGROUNDS_MSG));
	fBackgroundsButton->SetFontSize(be_plain_font->Size() * 0.9);
	fScreenBox->AddChild(fBackgroundsButton);

	fDefaultsButton = new BButton(buttonRect, "DefaultsButton", "Defaults",
		new BMessage(BUTTON_DEFAULTS_MSG));
	view->AddChild(fDefaultsButton);

	fRevertButton = new BButton(buttonRect, "RevertButton", "Revert",
		new BMessage(BUTTON_REVERT_MSG));
	fRevertButton->SetEnabled(false);
	view->AddChild(fRevertButton);

	fApplyButton = new BButton(buttonRect, "ApplyButton", "Apply", 
		new BMessage(BUTTON_APPLY_MSG));
	fApplyButton->SetEnabled(false);
	view->AddChild(fApplyButton);

	UpdateControls();

	LayoutControls(controlsFlags);
}


ScreenWindow::~ScreenWindow()
{
	delete fSettings;
}


bool
ScreenWindow::QuitRequested()
{
	fSettings->SetWindowFrame(Frame());
	if (fVesaApplied) {
		status_t status = _WriteVesaModeFile(fSelected);
		if (status < B_OK) {
			BString warning = "Could not write VESA mode settings file:\n\t";
			warning << strerror(status);
			(new BAlert("VesaAlert", warning.String(), "Okay", NULL, NULL,
				B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();

		}
	}

	be_app->PostMessage(B_QUIT_REQUESTED);

	return BWindow::QuitRequested();
}


/**	update resolution list according to combine mode
 *	(some resolution may not be combinable due to memory restrictions)
 */

void
ScreenWindow::CheckResolutionMenu()
{		
	for (int32 i = 0; i < fResolutionMenu->CountItems(); i++)
		fResolutionMenu->ItemAt(i)->SetEnabled(false);

	for (int32 i = 0; i < fScreenMode.CountModes(); i++) {
		screen_mode mode = fScreenMode.ModeAt(i);
		if (mode.combine != fSelected.combine)
			continue;

		BString name;
		name << mode.width << " x " << mode.height;

		BMenuItem *item = fResolutionMenu->FindItem(name.String());
		if (item != NULL)
			item->SetEnabled(true);
	}
}


/**	update color and refresh options according to current mode
 *	(a color space is made active if there is any mode with 
 *	given resolution and this colour space; same applies for 
 *	refresh rate, though "Other…" is always possible)
 */

void
ScreenWindow::CheckColorMenu()
{	
	for (int32 i = 0; i < kColorSpaceCount; i++) {
		bool supported = false;

		for (int32 j = 0; j < fScreenMode.CountModes(); j++) {
			screen_mode mode = fScreenMode.ModeAt(j);

			if (fSelected.width == mode.width
				&& fSelected.height == mode.height
				&& kColorSpaces[i].space == mode.space
				&& fSelected.combine == mode.combine) {
				supported = true;
				break;
			}
		}

		BMenuItem* item = fColorsMenu->ItemAt(i);
		if (item)
			item->SetEnabled(supported);
	}
}


/**	Enable/disable refresh options according to current mode. */

void
ScreenWindow::CheckRefreshMenu()
{
	float min, max;
	if (fScreenMode.GetRefreshLimits(fSelected, min, max) != B_OK || min == max)
		return;

	for (int32 i = fRefreshMenu->CountItems(); i-- > 0;) {
		BMenuItem* item = fRefreshMenu->ItemAt(i);
		BMessage* message = item->Message();
		float refresh;
		if (message != NULL && message->FindFloat("refresh", &refresh) == B_OK)
			item->SetEnabled(refresh >= min && refresh <= max);
	}
}


/** Activate appropriate menu item according to selected refresh rate */

void
ScreenWindow::UpdateRefreshControl()
{
	BString string;
	refresh_rate_to_string(fSelected.refresh, string);

	BMenuItem* item = fRefreshMenu->FindItem(string.String());
	if (item) {
		if (!item->IsMarked())
			item->SetMarked(true);

		// "Other…" items only contains a refresh rate when active
		fOtherRefresh->SetLabel("Other…");
		return;
	}

	// this is a non-standard refresh rate

	fOtherRefresh->Message()->ReplaceFloat("refresh", fSelected.refresh);
	fOtherRefresh->SetMarked(true);

	fRefreshMenu->Superitem()->SetLabel(string.String());

	string.Append("/Other" B_UTF8_ELLIPSIS);
	fOtherRefresh->SetLabel(string.String());
}


void
ScreenWindow::UpdateMonitorView()
{
	BMessage updateMessage(UPDATE_DESKTOP_MSG);
	updateMessage.AddInt32("width", fSelected.width);
	updateMessage.AddInt32("height", fSelected.height);

	PostMessage(&updateMessage, fMonitorView);
}


void
ScreenWindow::UpdateControls()
{
	BMenuItem* item = fSwapDisplaysMenu->ItemAt((int32)fSelected.swap_displays);
	if (item && !item->IsMarked())
		item->SetMarked(true);
	
	item = fUseLaptopPanelMenu->ItemAt((int32)fSelected.use_laptop_panel);
	if (item && !item->IsMarked())
		item->SetMarked(true);

	for (int32 i = 0; i < fTVStandardMenu->CountItems(); i++) {
		item = fTVStandardMenu->ItemAt(i);

		uint32 tvStandard;
		item->Message()->FindInt32("tv_standard", (int32 *)&tvStandard);
		if (tvStandard == fSelected.tv_standard) {
			if (!item->IsMarked())
				item->SetMarked(true);
			break;
		}
	}

	CheckResolutionMenu();
	CheckColorMenu();
	CheckRefreshMenu();

	BString string;
	resolution_to_string(fSelected, string);
	item = fResolutionMenu->FindItem(string.String());

	if (item != NULL) {
		if (!item->IsMarked())
			item->SetMarked(true);
	} else {
		// this is bad luck - if mode has been set via screen references, 
		// this case cannot occur; there are three possible solutions:
		// 1. add a new resolution to list
		//    - we had to remove it as soon as a "valid" one is selected
		//    - we don't know which frequencies/bit depths are supported
		//    - as long as we haven't the GMT formula to create 
		//      parameters for any resolution given, we cannot
		//      really set current mode - it's just not in the list
		// 2. choose nearest resolution
		//    - probably a good idea, but implies coding and testing
		// 3. choose lowest resolution
		//    - do you really think we are so lazy? yes, we are
		item = fResolutionMenu->ItemAt(0);
		if (item)
			item->SetMarked(true);

		// okay - at least we set menu label to active resolution
		fResolutionMenu->Superitem()->SetLabel(string.String());
	}

	// mark active combine mode
	for (int32 i = 0; i < kCombineModeCount; i++) {
		if (kCombineModes[i].mode == fSelected.combine) {
			item = fCombineMenu->ItemAt(i);
			if (item && !item->IsMarked())
				item->SetMarked(true);
			break;
		}
	}

	item = fColorsMenu->ItemAt(0);

	for (int32 i = kColorSpaceCount; i-- > 0;) {
		if (kColorSpaces[i].space == fSelected.space) {
			item = fColorsMenu->ItemAt(i);
			break;
		}
	}

	if (item && !item->IsMarked())
		item->SetMarked(true);

	string.Truncate(0);
	string << fSelected.BitsPerPixel() << " Bits/Pixel";
	if (string != fColorsMenu->Superitem()->Label())
		fColorsMenu->Superitem()->SetLabel(string.String());

	UpdateMonitorView();
	UpdateRefreshControl();

	CheckApplyEnabled();
}


/*! Reflect active mode in chosen settings */
void
ScreenWindow::UpdateActiveMode()
{
	// Usually, this function gets called after a mode
	// has been set manually; still, as the graphics driver
	// is free to fiddle with mode passed, we better ask 
	// what kind of mode we actually got
	if (!fVesaApplied)
		fScreenMode.Get(fActive);
	fSelected = fActive;

	UpdateControls();
}


void
ScreenWindow::ScreenChanged(BRect frame, color_space mode)
{
	// move window on screen, if necessary
	if (frame.right <= Frame().right
		&& frame.bottom <= Frame().bottom) {
		MoveTo((frame.Width() - Frame().Width()) / 2,
			(frame.Height() - Frame().Height()) / 2);
	}
}


void
ScreenWindow::WorkspaceActivated(int32 workspace, bool state)
{
	if (fChangingAllWorkspaces) {
		// we're currently changing all workspaces, so there is no need
		// to update the interface
		return;
	}

	fScreenMode.Get(fOriginal);
	fScreenMode.UpdateOriginalMode();

	// only override current settings if they have not been changed yet
	if (fSelected == fActive)
		UpdateActiveMode();

	BMessage message(UPDATE_DESKTOP_COLOR_MSG);
	PostMessage(&message, fMonitorView);
}


void
ScreenWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case WORKSPACE_CHECK_MSG:
			CheckApplyEnabled();
			break;

		case POP_WORKSPACE_CHANGED_MSG:
		{
			int32 index;
			if (message->FindInt32("index", &index) == B_OK)
				set_workspace_count(index + 1);
			break;
		}

		case POP_RESOLUTION_MSG:
		{
			int32 oldWidth = fSelected.width, oldHeight = fSelected.height;
			message->FindInt32("width", &fSelected.width);
			message->FindInt32("height", &fSelected.height);
			if (fSelected.width == oldWidth && fSelected.height == oldHeight)
				break;

			CheckColorMenu();
			CheckRefreshMenu();

			UpdateMonitorView();
			UpdateRefreshControl();

			fScreenMode.Get(fOriginal);
			fScreenMode.UpdateOriginalMode();
			CheckApplyEnabled();
			break;
		}

		case POP_COLORS_MSG:
		{
			color_space old = fSelected.space;
			message->FindInt32("space", (int32 *)&fSelected.space);
			if (fSelected.space == old)
				break;

			BString string;
			string << fSelected.BitsPerPixel() << " Bits/Pixel";
			fColorsMenu->Superitem()->SetLabel(string.String());

			fScreenMode.Get(fOriginal);
			fScreenMode.UpdateOriginalMode();
			CheckApplyEnabled();
			break;
		}

		case POP_REFRESH_MSG:
		{
			float old = fSelected.refresh;
			message->FindFloat("refresh", &fSelected.refresh);
			fOtherRefresh->SetLabel("Other" B_UTF8_ELLIPSIS);
				// revert "Other…" label - it might have had a refresh rate prefix

			if (fSelected.refresh == old)
				break;

			fScreenMode.Get(fOriginal);
			fScreenMode.UpdateOriginalMode();
			CheckApplyEnabled();		
			break;
		}

		case POP_OTHER_REFRESH_MSG:
		{
			// make sure menu shows something useful
			UpdateRefreshControl();

			float min = 0, max = 999;
			fScreenMode.GetRefreshLimits(fSelected, min, max);
			if (min < gMinRefresh)
				min = gMinRefresh;
			if (max > gMaxRefresh)
				max = gMaxRefresh;

			RefreshWindow *fRefreshWindow = new RefreshWindow(
				fRefreshField->ConvertToScreen(B_ORIGIN), fSelected.refresh, min, max);
			fRefreshWindow->Show();
			break;
		}

		case SET_CUSTOM_REFRESH_MSG:
		{
			// user pressed "done" in "Other…" refresh dialog;
			// select the refresh rate chosen
			float old = fSelected.refresh;
			message->FindFloat("refresh", &fSelected.refresh);
			if (fSelected.refresh != old)
				break;

			fScreenMode.Get(fOriginal);
			fScreenMode.UpdateOriginalMode();
			UpdateRefreshControl();
			CheckApplyEnabled();
			break;
		}

		case POP_COMBINE_DISPLAYS_MSG:
		{
			// new combine mode has bee chosen
			int32 mode;
			if (message->FindInt32("mode", &mode) == B_OK)
				fSelected.combine = (combine_mode)mode;

			CheckResolutionMenu();
			CheckApplyEnabled();
			break;
		}
		
		case POP_SWAP_DISPLAYS_MSG:
			message->FindBool("swap", &fSelected.swap_displays);
			CheckApplyEnabled();
			break;

		case POP_USE_LAPTOP_PANEL_MSG:
			message->FindBool("use", &fSelected.use_laptop_panel);
			CheckApplyEnabled();
			break;

		case POP_TV_STANDARD_MSG:
			message->FindInt32("tv_standard", (int32 *)&fSelected.tv_standard);
			CheckApplyEnabled();
			break;

		case BUTTON_LAUNCH_BACKGROUNDS_MSG:
			if (be_roster->Launch(kBackgroundsSignature) == B_ALREADY_RUNNING) {
				app_info info;
				be_roster->GetAppInfo(kBackgroundsSignature, &info);
				be_roster->ActivateApp(info.team);
			}
			break;

		case BUTTON_DEFAULTS_MSG:
		{
			fSelected.width = 640;
			fSelected.height = 480;
			fSelected.space = B_CMAP8;
			fSelected.refresh = 60.0;
			fSelected.combine = kCombineDisable;
			fSelected.swap_displays = false;
			fSelected.use_laptop_panel = false;
			fSelected.tv_standard = 0;

			fScreenMode.Get(fOriginal);
			fScreenMode.UpdateOriginalMode();
			UpdateControls();
			break;
		}

		case BUTTON_REVERT_MSG:
		case SET_INITIAL_MODE_MSG:
			fScreenMode.Revert();
			UpdateActiveMode();
			break;

		case BUTTON_APPLY_MSG:
			Apply();
			break;

		case MAKE_INITIAL_MSG: {
			// user pressed "keep" in confirmation box;
			// select this mode in dialog and mark it as
			// previous mode; if "all workspaces" is selected, 
			// distribute mode to all workspaces 

			// use the mode that has eventually been set and
			// thus we know to be working; it can differ from 
			// the mode selected by user due to hardware limitation
			display_mode newMode;
			BScreen screen(this);
			screen.GetMode(&newMode);

			if (fAllWorkspacesItem->IsMarked()) {
				int32 originatingWorkspace;

				// the original panel activates each workspace in turn;
				// this is disguisting and there is a SetMode
				// variant that accepts a workspace id, so let's take
				// this one
				originatingWorkspace = current_workspace();

				// well, this "cannot be reverted" message is not
				// entirely true - at least, you can revert it
				// for current workspace; to not overwrite original
				// mode during workspace switch, we use this flag
				fChangingAllWorkspaces = true;

				for (int32 i = 0; i < count_workspaces(); i++) {
					if (i != originatingWorkspace)
						screen.SetMode(i, &newMode, true);
				}

				fChangingAllWorkspaces = false;
			}

			UpdateActiveMode();
			break;
		}
		
		default:
			BWindow::MessageReceived(message);		
			break;
	}
}


status_t
ScreenWindow::_WriteVesaModeFile(const screen_mode& mode) const
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
	if (status < B_OK)
		return status;

	path.Append("kernel/drivers");
	status = create_directory(path.Path(), 0755);
	if (status < B_OK)
		return status;

	path.Append("vesa");
	BFile file;
	status = file.SetTo(path.Path(), B_CREATE_FILE | B_WRITE_ONLY | B_ERASE_FILE);
	if (status < B_OK)
		return status;

	char buffer[256];
	snprintf(buffer, sizeof(buffer), "mode %ld %ld %ld\n",
		mode.width, mode.height, mode.BitsPerPixel());

	ssize_t bytesWritten = file.Write(buffer, strlen(buffer));
	if (bytesWritten < B_OK)
		return bytesWritten;

	return B_OK;
}


bool
ScreenWindow::CanApply() const
{
	return fAllWorkspacesItem->IsMarked() || fSelected != fActive
			|| (fSelected == fOriginal && fActive != fOriginal);
}


bool
ScreenWindow::CanRevert() const
{
	return (fActive != fOriginal && !fAllWorkspacesItem->IsMarked())
			|| fSelected != fActive;
}


void
ScreenWindow::CheckApplyEnabled()
{
	fApplyButton->SetEnabled(CanApply());
	fRevertButton->SetEnabled(CanRevert());
}


void
ScreenWindow::Apply()
{
	if (_IsVesa()) {
		(new BAlert("VesaAlert",
			"Your graphics card is not supported. Resolution changes will be "
			"adopted on next system startup.\n", "Okay", NULL, NULL, B_WIDTH_AS_USUAL,
			B_INFO_ALERT))->Go(NULL);

		fVesaApplied = true;
		fActive = fSelected;
		return;
	}

	if (fAllWorkspacesItem->IsMarked()) {
		BAlert *workspacesAlert = new BAlert("WorkspacesAlert",
			"Change all workspaces? This action cannot be reverted.", "Okay", "Cancel", 
			NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);

		if (workspacesAlert->Go() == 1)
			return;
	}

	status_t status = fScreenMode.Set(fSelected);
	if (status == B_OK) {
		fActive = fSelected;

		// ToDo: only show alert when this is an unknown mode
		BWindow* window = new AlertWindow(this);
		window->Show();
	} else {
		char message[256];
		snprintf(message, sizeof(message),
			"The screen mode could not be set:\n\t%s\n", screen_errors(status));
		BAlert* alert = new BAlert("Screen Alert", message, "Okay", NULL, NULL,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go();
	}
}


void
ScreenWindow::LayoutControls(uint32 flags)
{
	// layout the screen box and its controls
	fWorkspaceCountField->ResizeToPreferred();

	float monitorViewHeight = fMonitorView->Bounds().Height();
	float workspaceFieldHeight = fWorkspaceCountField->Bounds().Height();
	float backgroundsButtonHeight = fBackgroundsButton->Bounds().Height();

	float screenBoxWidth = fWorkspaceCountField->Bounds().Width() + 20.0;
	float screenBoxHeight = monitorViewHeight + 5.0
							+ workspaceFieldHeight + 5.0
							+ backgroundsButtonHeight
							+ 20.0;

#ifdef __HAIKU__
	fScreenBox->MoveTo(10.0, 10.0 + fControlsBox->TopBorderOffset());
#else
	fScreenBox->MoveTo(10.0, 10.0 + 3);
#endif
	fScreenBox->ResizeTo(screenBoxWidth, screenBoxHeight);

	float leftOffset = 10.0;
	float topOffset = 10.0;
	fMonitorView->MoveTo(leftOffset, topOffset);
	fMonitorView->ResizeTo(screenBoxWidth - 20.0, monitorViewHeight);
	fMonitorView->SetResizingMode(B_FOLLOW_ALL);
	topOffset += monitorViewHeight + 5.0;

	fWorkspaceCountField->MoveTo(leftOffset, topOffset);
	fWorkspaceCountField->SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	topOffset += workspaceFieldHeight + 5.0;

	fBackgroundsButton->MoveTo(leftOffset, topOffset);
	fBackgroundsButton->ResizeTo(screenBoxWidth - 20.0, backgroundsButtonHeight);
	fBackgroundsButton->SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);

	fControlsBox->MoveTo(fScreenBox->Frame().right + 10.0, 10.0);

	// layout the right side
	BRect controlsRect = LayoutMenuFields(flags);
	controlsRect.InsetBy(-10.0, -10.0);
	// adjust size of controls box and move aligned buttons along
	float xDiff = controlsRect.right - fControlsBox->Bounds().right;
	float yDiff = controlsRect.bottom - fControlsBox->Bounds().bottom;
	if (yDiff < 0.0) {
		// don't shrink vertically
		yDiff = 0.0;
	}

	fControlsBox->ResizeBy(xDiff, yDiff);

	// align bottom of boxen
	float boxBottomDiff = fControlsBox->Frame().bottom - fScreenBox->Frame().bottom;
	if (boxBottomDiff > 0)
		fScreenBox->ResizeBy(0.0, boxBottomDiff);
	else
		fControlsBox->ResizeBy(0.0, -boxBottomDiff);

	BRect boxFrame = fScreenBox->Frame() | fControlsBox->Frame();

	// layout rest of buttons
	fDefaultsButton->ResizeToPreferred();
	fDefaultsButton->MoveTo(boxFrame.left, boxFrame.bottom + 8);

	fRevertButton->ResizeToPreferred();
	fRevertButton->MoveTo(fDefaultsButton->Frame().right + 10,
						  fDefaultsButton->Frame().top);

	fApplyButton->ResizeToPreferred();
	fApplyButton->MoveTo(boxFrame.right - fApplyButton->Bounds().Width(),
						 fDefaultsButton->Frame().top);

	ResizeTo(boxFrame.right + 10, fDefaultsButton->Frame().bottom + 10);
}


BRect
ScreenWindow::LayoutMenuFields(uint32 flags, bool sideBySide)
{
	BList menuFields;
	menuFields.AddItem((void*)fResolutionField);
	menuFields.AddItem((void*)fColorsField);
	menuFields.AddItem((void*)fRefreshField);

	BRect frame;

	if (sideBySide)
		frame = stack_and_align_menu_fields(menuFields);

	if (sideBySide)
		menuFields.MakeEmpty();

	if (flags & SHOW_COMBINE_FIELD)
		menuFields.AddItem((void*)fCombineField);
	if (flags & SHOW_SWAP_FIELD)
		menuFields.AddItem((void*)fSwapDisplaysField);
	if (flags & SHOW_LAPTOP_PANEL_FIELD)
		menuFields.AddItem((void*)fUseLaptopPanelField);
	if (flags & SHOW_TV_STANDARD_FIELD)
		menuFields.AddItem((void*)fTVStandardField);

	if (sideBySide) {
		if (menuFields.CountItems() > 0) {
			((BMenuField*)menuFields.FirstItem())->MoveTo(frame.right + 8.0, frame.top);
			frame = frame | stack_and_align_menu_fields(menuFields);
		}
	} else {
		frame = stack_and_align_menu_fields(menuFields);
	}

	return frame;
}

