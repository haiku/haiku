/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Rafael Romo
 *		Stefano Ceccherini (burton666@libero.it)
 *		Andrew Bachmann
 *		Thomas Kurschel
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <Alert.h>
#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <InterfaceDefs.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <String.h>
#include <Window.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

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


#define USE_FIXED_REFRESH
	// define to use fixed standard refresh rates
	// undefine to get standard refresh rates from driver


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
static const int32 kRefreshRates[] = {56, 60, 70, 72, 75};
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


//	#pragma mark -


ScreenWindow::ScreenWindow(ScreenSettings *settings)
	: BWindow(settings->WindowFrame(), "Screen", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE, B_ALL_WORKSPACES),
	fScreenMode(this),
	fChangingAllWorkspaces(false)
{
	BScreen screen(this);
	BRect frame(Bounds());

	fScreenMode.Get(fOriginal);
	fActive = fSelected = fOriginal;

	BView *view = new BView(frame, "ScreenView", B_FOLLOW_ALL, B_WILL_DRAW);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);

	fSettings = settings;

	// we need the "Current Workspace" first to get its height

	BPopUpMenu *popUpMenu = new BPopUpMenu("Current Workspace", true, true);
	fAllWorkspacesItem = new BMenuItem("All Workspaces", new BMessage(WORKSPACE_CHECK_MSG));
	popUpMenu->AddItem(fAllWorkspacesItem);
	BMenuItem *item = new BMenuItem("Current Workspace", new BMessage(WORKSPACE_CHECK_MSG));
	item->SetMarked(true);
	popUpMenu->AddItem(item);

	BMenuField* workspaceMenuField = new BMenuField(BRect(0, 0, 125, 18),
		"WorkspaceMenu", NULL, popUpMenu, true);
	workspaceMenuField->ResizeToPreferred();

	// box on the left with workspace count and monitor view

	float labelWidth = be_plain_font->StringWidth("Workspaces count:") + 5.0f;

	BRect rect(10.0, 7.0 + workspaceMenuField->Bounds().Height() / 2.0f,
		26 + labelWidth + 32, 155.0);
	BBox *screenBox = new BBox(rect, "left box");

	rect = screenBox->Bounds().InsetByCopy(8, 8);
	rect.bottom = rect.top + workspaceMenuField->Bounds().Height();
	popUpMenu = new BPopUpMenu("", true, true);
	BMenuField *menuField = new BMenuField(rect, "WorkspaceCountMenu",
		"Workspace count:", popUpMenu, true);
	float width, height;
	menuField->GetPreferredSize(&width, &height);
	menuField->ResizeTo(rect.Width(), height);
	menuField->MoveTo(rect.left, screenBox->Bounds().bottom - 8 - height);
	menuField->SetDivider(labelWidth);
	screenBox->AddChild(menuField);

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

	rect = screenBox->Bounds().InsetByCopy(8, 8);
	rect.bottom -= height - 4;
	fMonitorView = new MonitorView(rect, "monitor",
		screen.Frame().Width() + 1, screen.Frame().Height() + 1);
	screenBox->AddChild(fMonitorView);

	view->AddChild(screenBox);

	// box on the right with screen resolution, etc.

	rect = screenBox->Frame();
	rect.top = 7.0f;
	rect.left = rect.right + 10.0f;
	rect.right = rect.left + 190.0f;
	BBox* controlsBox = new BBox(rect);
	controlsBox->SetLabel(workspaceMenuField);

	rect.SetLeftTop(controlsBox->Bounds().RightBottom());
	fApplyButton = new BButton(rect, "ApplyButton", "Apply", 
		new BMessage(BUTTON_APPLY_MSG), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fApplyButton->ResizeToPreferred();
	fApplyButton->MoveTo(rect.LeftTop() - BPoint(8, 8)
		- fApplyButton->Bounds().RightBottom());
	fApplyButton->SetEnabled(false);
	controlsBox->AddChild(fApplyButton);

	labelWidth = controlsBox->StringWidth("Refresh rate:") + 5.0f;
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

	rect.Set(10.0, 30.0, 179.0, 48.0);
	fResolutionField = new BMenuField(rect, "ResolutionMenu", "Resolution:",
		fResolutionMenu, true);
	fResolutionField->SetAlignment(B_ALIGN_RIGHT);
	fResolutionField->SetDivider(labelWidth);
	controlsBox->AddChild(fResolutionField);

	fColorsMenu = new BPopUpMenu("colors", true, true);

	for (int32 i = 0; i < kColorSpaceCount; i++) {
		BMessage *message = new BMessage(POP_COLORS_MSG);
		message->AddInt32("bits_per_pixel", kColorSpaces[i].bits_per_pixel);
		message->AddInt32("space", kColorSpaces[i].space);

		fColorsMenu->AddItem(new BMenuItem(kColorSpaces[i].label, message));
	}

	rect.Set(10.0, 58.0, 179.0, 76.0);
	fColorsField = new BMenuField(rect, "ColorsMenu", "Colors:", fColorsMenu, true);
	fColorsField->SetAlignment(B_ALIGN_RIGHT);
	fColorsField->SetDivider(labelWidth);
	controlsBox->AddChild(fColorsField);

	fRefreshMenu = new BPopUpMenu("refresh rate", true, true);

#ifdef USE_FIXED_REFRESH
	for (int32 i = 0; i < kRefreshRateCount; ++i) {
		BString name;
		name << kRefreshRates[i] << " Hz";

		BMessage *message = new BMessage(POP_REFRESH_MSG);
		message->AddFloat("refresh", kRefreshRates[i]);

		fRefreshMenu->AddItem(new BMenuItem(name.String(), message));
	}
#endif

	BMessage *message = new BMessage(POP_OTHER_REFRESH_MSG);

	fOtherRefresh = new BMenuItem("Other" B_UTF8_ELLIPSIS, message);
	fRefreshMenu->AddItem(fOtherRefresh);

	rect.Set(10.0, 86.0, 179.0, 104.0);
	fRefreshField = new BMenuField(rect, "RefreshMenu", "Refresh Rate:", fRefreshMenu, true);
	fRefreshField->SetAlignment(B_ALIGN_RIGHT);
	fRefreshField->SetDivider(labelWidth);
	controlsBox->AddChild(fRefreshField);

	view->AddChild(controlsBox);

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

		// even if there is no support, we still create all controls
		// to make sure we don't access NULL pointers later on
		if (multiMonSupport)
			controlsBox->ResizeTo(366, 148);		

		fCombineMenu = new BPopUpMenu("CombineDisplays", true, true);

		for (int32 i = 0; i < kCombineModeCount; i++) {
			message = new BMessage(POP_COMBINE_DISPLAYS_MSG);
			message->AddInt32("mode", kCombineModes[i].mode);

			fCombineMenu->AddItem(new BMenuItem(kCombineModes[i].name, message));
		}

		rect.Set(185, 30, 356, 48);
		BMenuField* menuField = new BMenuField(rect, "CombineMenu",
			"Combine Displays:", fCombineMenu, true);
		menuField->SetDivider(90);
		controlsBox->AddChild(menuField);

		if (!multiMonSupport)
			menuField->Hide();

		fSwapDisplaysMenu = new BPopUpMenu("SwapDisplays", true, true);

		// !order is important - we rely that boolean value == idx
		message = new BMessage(POP_SWAP_DISPLAYS_MSG);
		message->AddBool("swap", false);
		fSwapDisplaysMenu->AddItem(new BMenuItem("no", message));

		message = new BMessage(POP_SWAP_DISPLAYS_MSG);
		message->AddBool("swap", true);
		fSwapDisplaysMenu->AddItem(new BMenuItem("yes", message));

		rect.Set(199, 58, 356, 76);
		menuField = new BMenuField(rect, "SwapMenu", "Swap Displays:",
			fSwapDisplaysMenu, true);
		menuField->SetDivider(76);

		controlsBox->AddChild(menuField);
		if (!multiMonSupport)
			menuField->Hide();

		fUseLaptopPanelMenu = new BPopUpMenu("UseLaptopPanel", true, true);

		// !order is important - we rely that boolean value == idx			
		message = new BMessage(POP_USE_LAPTOP_PANEL_MSG);
		message->AddBool("use", false);
		fUseLaptopPanelMenu->AddItem(new BMenuItem("if needed", message));

		message = new BMessage(POP_USE_LAPTOP_PANEL_MSG);
		message->AddBool("use", true);
		fUseLaptopPanelMenu->AddItem(new BMenuItem("always", message));

		rect.Set(184, 86, 356, 104);
		menuField = new BMenuField(rect, "UseLaptopPanel", "Use Laptop Panel:",
			fUseLaptopPanelMenu, true);
		menuField->SetDivider(91);

		controlsBox->AddChild(menuField);
		if (!useLaptopPanelSupport)
			menuField->Hide();

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

		rect.Set(15, 114, 171, 132);
		menuField = new BMenuField(rect, "tv standard", "Video Format:",
			fTVStandardMenu, true);
		menuField->SetDivider(73);

		if (!tvStandardSupport || i == 0)
			menuField->Hide();

		controlsBox->AddChild(menuField);
	}

	rect.Set(10.0, screenBox->Frame().bottom + 10.0, 100.0, 200.0);
	fDefaultsButton = new BButton(rect, "DefaultsButton", "Defaults",
		new BMessage(BUTTON_DEFAULTS_MSG));
	fDefaultsButton->ResizeToPreferred();
	view->AddChild(fDefaultsButton);

	rect.OffsetBy(fDefaultsButton->Bounds().Width() + 10, 0);
	fRevertButton = new BButton(rect, "RevertButton", "Revert",
		new BMessage(BUTTON_REVERT_MSG));
	fRevertButton->ResizeToPreferred();
	fRevertButton->SetEnabled(false);
	view->AddChild(fRevertButton);

	ResizeTo(controlsBox->Frame().right + 10,
		fDefaultsButton->Frame().bottom + 10);

	UpdateControls();
}


ScreenWindow::~ScreenWindow()
{
	delete fSettings;
}


bool
ScreenWindow::QuitRequested()
{
	fSettings->SetWindowFrame(Frame());
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


/**	Enable/disable refresh options according to current mode.
 *	Only needed when USE_FIXED_REFRESH is not defined.
 */

void
ScreenWindow::CheckRefreshMenu()
{	
#ifndef USE_FIXED_REFRESH
	// ToDo: does currently not compile!
	for (int32 i = fRefreshMenu->CountItems() - 2; i >= 0; --i) {
		delete fRefreshMenu->RemoveItem(i);
	}

	for (int32 i = 0; i < fModeListCount; ++i) {
		if (virtualWidth == fModeList[i].virtual_width
			&& virtualHeight == fModeList[i].virtual_height
			&& combine == get_combine_mode(&fModeList[i])) {
			BString name;
			BMenuItem *item;

			int32 refresh10 = get_refresh10(fModeList[i]);
			refresh10_to_string(name, refresh10);

			item = fRefreshMenu->FindItem(name.String());
			if (item == NULL) {
				BMessage *msg = new BMessage(POP_REFRESH_MSG);
				msg->AddFloat("refresh", refresh);

				fRefreshMenu->AddItem(new BMenuItem(name.String(), msg),
					fRefreshMenu->CountItems() - 1);
			}
		}
	}
#endif			

	// TBD: some drivers lack many refresh rates; still, they
	// can be used by generating the mode manually
/*
	for( i = 0; i < sizeof( refresh_list ) / sizeof( refresh_list[0] ); ++i ) {
		BMenuItem *item;
		bool supported = false;
		
		for( j = 0; j < fModeListCount; ++j ) {
			if( width == fModeList[j].virtual_width &&
				height == fModeList[j].virtual_height &&
				refresh_list[i].refresh * 10 == getModeRefresh10( &fModeList[j] ))
			{
				supported = true;
				break;
			}
		}

		item = fRefreshMenu->ItemAt( i );
		if( item ) 
			item->SetEnabled( supported );
	}
*/
}


/** activate appropriate menu item according to selected refresh rate */

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


/** reflect active mode in chosen settings */

void
ScreenWindow::UpdateActiveMode()
{
	// usually, this function gets called after a mode
	// has been set manually; still, as the graphics driver
	// is free to fiddle with mode passed, we better ask 
	// what kind of mode we actually got
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
			message->FindInt32("width", &fSelected.width);
			message->FindInt32("height", &fSelected.height);

			CheckColorMenu();
			CheckRefreshMenu();

			UpdateMonitorView();
			UpdateRefreshControl();

			CheckApplyEnabled();
			break;
		}

		case POP_COLORS_MSG:
		{
			message->FindInt32("space", (int32 *)&fSelected.space);

			BString string;
			string << fSelected.BitsPerPixel() << " Bits/Pixel";
			fColorsMenu->Superitem()->SetLabel(string.String());

			CheckApplyEnabled();
			break;
		}

		case POP_REFRESH_MSG:
			message->FindFloat("refresh", &fSelected.refresh);
			fOtherRefresh->SetLabel("Other" B_UTF8_ELLIPSIS);
				// revert "Other…" label - it might have had a refresh rate prefix

			CheckApplyEnabled();		
			break;

		case POP_OTHER_REFRESH_MSG:
		{
			// make sure menu shows something usefull
			UpdateRefreshControl();

			BRect frame(Frame());
			RefreshWindow *fRefreshWindow = new RefreshWindow(BRect(frame.left + 201.0,
				frame.top + 34.0, frame.left + 509.0, frame.top + 169.0),
				int32(fSelected.refresh * 10));
			fRefreshWindow->Show();
			break;
		}

		case SET_CUSTOM_REFRESH_MSG:
		{
			// user pressed "done" in "Other…" refresh dialog;
			// select the refresh rate chosen
			message->FindFloat("refresh", &fSelected.refresh);

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


bool
ScreenWindow::CanApply() const
{
	if (fAllWorkspacesItem->IsMarked())
		return true;

	return fSelected != fActive;
}


bool
ScreenWindow::CanRevert() const
{
	if (fActive != fOriginal)
		return true;

	return CanApply();
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

