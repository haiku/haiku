#include <Alert.h>
#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <InterfaceDefs.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <String.h>
#include <Screen.h>
#include <Window.h>

#include <cstring>
#include <cstdlib>
#include <cstdio>

#include "RefreshWindow.h"
#include "ScreenWindow.h"
#include "ScreenDrawView.h"
#include "ScreenSettings.h"
#include "AlertWindow.h"
#include "Constants.h"
#include "Utility.h"

static uint32
colorspace_to_bpp(uint32 colorspace)
{
	switch (colorspace) {
	case B_RGB32:	return 32;
	case B_RGB24:	return 24;
	case B_RGB16:	return 16;
	case B_RGB15:	return 15;
	case B_CMAP8:	return 8;
	default:		return 0;
	}
}

static void
colorspace_to_string(uint32 colorspace, char dest[])
{
	sprintf(dest,"%lu Bits/Pixel",colorspace_to_bpp(colorspace));
}

static uint32
string_to_colorspace(const char* string)
{
	uint32 bits = 0;
	sscanf(string,"%ld",&bits);
	switch (bits) {
	case 8:		return B_CMAP8;
	case 15:	return B_RGB15;
	case 16:	return B_RGB16;
	case 24:	return B_RGB24;
	case 32:	return B_RGB32;
	default:	return B_CMAP8; // Should return an error?
	}
}


static void
mode_to_string(display_mode mode, char dest[])
{
	sprintf(dest, "%hu x %hu", mode.virtual_width, mode.virtual_height);
}


static void
string_to_mode(const char * source, display_mode * mode)
{
	sscanf(source,"%hu x %hu", &(mode->virtual_width), &(mode->virtual_height));
}


static void
refresh_rate_to_string(float refresh_rate, char dest[])
{
	sprintf(dest, "%.1f Hz",refresh_rate);
}


/*
static float
string_to_refresh_rate(const char * source)
{
	float refresh_rate = 0;
	sscanf(source,"%f Hz", &refresh_rate);
	return refresh_rate;
}
*/

static float
get_refresh_rate(display_mode mode)
{
	return (mode.timing.pixel_clock * 1000.0) / (mode.timing.h_total * mode.timing.v_total);
}


static void
set_pixel_clock(display_mode * mode, float refresh_rate)
{
	mode->timing.pixel_clock
	    = (uint32)((mode->timing.h_total * mode->timing.v_total * refresh_rate) / 1000);
}


ScreenWindow::ScreenWindow(ScreenSettings *Settings)
	: BWindow(Settings->WindowFrame(), "Screen", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE, B_ALL_WORKSPACES)
{
	BRect frame(Bounds());
	BScreen screen(B_MAIN_SCREEN_ID);
	
	if (!screen.IsValid())
		;//debugger() ?
		
	screen.GetModeList(&fSupportedModes, &fTotalModes);
	
	frame.InsetBy(-1, -1);
	fScreenView = new BBox(frame, "ScreenView");
	fScreenDrawView = new ScreenDrawView(BRect(20.0, 16.0, 122.0, 93.0), "ScreenDrawView");	
		
	AddChild(fScreenView);
	
	fSettings = Settings;
	
	BRect screenBoxRect(11.0, 18.0, 153.0, 155.0);	
	BBox *screenBox = new BBox(screenBoxRect);
	screenBox->SetBorder(B_FANCY_BORDER);
	
	fWorkspaceCountMenu = new BPopUpMenu("", true, true);
	fWorkspaceCountField = new BMenuField(BRect(7.0, 107.0, 135.0, 127.0), "WorkspaceCountMenu", "Workspace count:", fWorkspaceCountMenu, true);

	for (int32 count = 1; count <= 32; count++) {
		BString workspaceCount;
		workspaceCount << count;
		fWorkspaceCountMenu->AddItem(new BMenuItem(workspaceCount.String(),
			new BMessage(POP_WORKSPACE_CHANGED_MSG)));
	}

	BString string;
	string << count_workspaces();
	
	BMenuItem *marked = fWorkspaceCountMenu->FindItem(string.String());
	marked->SetMarked(true);
			
	fWorkspaceCountField->SetDivider(91.0);
	
	screenBox->AddChild(fScreenDrawView);	
	screenBox->AddChild(fWorkspaceCountField);	
	fScreenView->AddChild(screenBox);
	
	fWorkspaceMenu = new BPopUpMenu("Current Workspace", true, true);
	fAllWorkspacesItem = new BMenuItem("All Workspaces", new BMessage(WORKSPACE_CHECK_MSG));
	fWorkspaceMenu->AddItem(fAllWorkspacesItem);
	fCurrentWorkspaceItem = new BMenuItem("Current Workspace", new BMessage(WORKSPACE_CHECK_MSG));
	fCurrentWorkspaceItem->SetMarked(true);
	fWorkspaceMenu->AddItem(fCurrentWorkspaceItem);
	
	BRect workspaceMenuRect(0.0, 0.0, 132.0, 18.0);
	fWorkspaceField = new BMenuField(workspaceMenuRect, "WorkspaceMenu", NULL, fWorkspaceMenu, true);
	
	BRect controlsBoxRect(164.0, 7.0, 345.0, 155.0);	
	BBox *controlsBox = new BBox(controlsBoxRect);
	controlsBox->SetBorder(B_FANCY_BORDER);
	controlsBox->SetLabel(fWorkspaceField);
	
	BRect ButtonRect(88.0, 114.0, 200.0, 150.0);	
	fApplyButton = new BButton(ButtonRect, "ApplyButton", "Apply", 
		new BMessage(BUTTON_APPLY_MSG));
	
	fApplyButton->AttachedToWindow();
	fApplyButton->ResizeToPreferred();
	fApplyButton->SetEnabled(false);
	
	controlsBox->AddChild(fApplyButton);
	
	fResolutionMenu = new BPopUpMenu("", true, true);	
	fColorsMenu = new BPopUpMenu("", true, true);
	fRefreshMenu = new BPopUpMenu("", true, true);
	
	CheckUpdateDisplayModes();
	
	const char * resolutionLabel = "Resolution: ";
	float resolutionWidth = controlsBox->StringWidth(resolutionLabel);
	BRect controlMenuRect(88.0-resolutionWidth, 30.0, 171.0, 48.0);	
	fResolutionField = new BMenuField(controlMenuRect, "ResolutionMenu", resolutionLabel, fResolutionMenu, true);
	
	marked = fResolutionMenu->ItemAt(0);
	marked->SetMarked(true);
	
	fResolutionField->SetDivider(resolutionWidth);
	
	controlsBox->AddChild(fResolutionField);
	
	const char * colorsLabel = "Colors: ";
	float colorsWidth = controlsBox->StringWidth(colorsLabel);
	controlMenuRect.Set(88.0-colorsWidth, 58.0, 171.0, 76.0);
	fColorsField = new BMenuField(controlMenuRect, "ColorsMenu", colorsLabel, fColorsMenu, true);
	
	marked = fColorsMenu->ItemAt(0);
	marked->SetMarked(true);
	
	fColorsField->SetDivider(colorsWidth);
	
	controlsBox->AddChild(fColorsField);
	
	fOtherRefresh = new BMenuItem("Other...", new BMessage(POP_OTHER_REFRESH_MSG));
	fRefreshMenu->AddItem(fOtherRefresh);
	
	const char * refreshLabel = "Refresh Rate: ";
	float refreshWidth = controlsBox->StringWidth(refreshLabel);
	controlMenuRect.Set(88.0-refreshWidth, 86.0, 171.0, 104.0);
	fRefreshField = new BMenuField(controlMenuRect, "RefreshMenu", refreshLabel, fRefreshMenu, true);
	
	fRefreshField->SetDivider(refreshWidth);
	
	controlsBox->AddChild(fRefreshField);
	
	fScreenView->AddChild(controlsBox);
	
	ButtonRect.Set(10.0, 167, 100.0, 200.0);
	
	fDefaultsButton = new BButton(ButtonRect, "DefaultsButton", "Defaults", 
		new BMessage(BUTTON_DEFAULTS_MSG));
	
	fDefaultsButton->AttachedToWindow();
	fDefaultsButton->ResizeToPreferred();
	
	fScreenView->AddChild(fDefaultsButton);
	
	ButtonRect.Set(95.0, 167, 160.0, 200.0);
	
	fRevertButton = new BButton(ButtonRect, "RevertButton", "Revert", 
		new BMessage(BUTTON_REVERT_MSG));
	
	fRevertButton->AttachedToWindow();
	fRevertButton->ResizeToPreferred();
	fRevertButton->SetEnabled(false);
	
	fScreenView->AddChild(fRevertButton);
	
	SetStateByMode();

	fCustomRefresh = fInitialRefreshN;
}

void
ScreenWindow::SetStateByMode()
{	
	BString string;
	BMenuItem *marked;
	display_mode mode;
	BScreen screen(B_MAIN_SCREEN_ID);
	
	if (!screen.IsValid()) {
		//debugger() ?
		return;
	}

	screen.GetMode(&mode);
	fInitialMode = mode;
	
	char str[256];
	mode_to_string(mode,str);
	marked = fResolutionMenu->FindItem(str);
	marked->SetMarked(true);
	fInitialResolution = marked;
	
	colorspace_to_string(mode.space,str);
	marked = fColorsMenu->FindItem(str);
	marked->SetMarked(true);
	fInitialColors = marked;
	
	fInitialRefreshN = get_refresh_rate(mode);
	refresh_rate_to_string(fInitialRefreshN,str);
	marked = fRefreshMenu->FindItem(str);
	
	if (marked != NULL)	{	
		marked->SetMarked(true);	
		fInitialRefresh = marked;	
	} else 	{
		BString string(str);
		string << "/Other...";
	
		fOtherRefresh->SetLabel(string.String());
		fOtherRefresh->SetMarked(true);
		
		fRefreshMenu->Superitem()->SetLabel(str);
		
		fInitialRefresh = fOtherRefresh;
		fCustomRefresh = fInitialRefreshN;
	}
}


ScreenWindow::~ScreenWindow()
{
	delete fSettings;
	delete[] fSupportedModes;
}


bool
ScreenWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	
	return BWindow::QuitRequested();
}


void
ScreenWindow::FrameMoved(BPoint position)
{
	fSettings->SetWindowFrame(Frame());
}


void
ScreenWindow::ScreenChanged(BRect frame, color_space mode)
{
	if (frame.right <= Frame().right
				&& frame.bottom <= Frame().bottom)
		MoveTo(((frame.right / 2) - 178), ((frame.right / 2) - 101));
}


void
ScreenWindow::WorkspaceActivated(int32 ws, bool state)
{
	PostMessage(new BMessage(UPDATE_DESKTOP_COLOR_MSG), fScreenDrawView);
}


void
ScreenWindow::MessageReceived(BMessage* message)
{
	switch(message->what)
	{
		case WORKSPACE_CHECK_MSG:
		{
			fApplyButton->SetEnabled(true);
			
			break;
		}
	
		case POP_WORKSPACE_CHANGED_MSG:
		{		
			BMenuItem *item = fWorkspaceCountMenu->FindMarked();
		
			set_workspace_count(fWorkspaceCountMenu->IndexOf(item) + 1);
		
			break;
		}
		
		case POP_RESOLUTION_MSG:
		{
			CheckApplyEnabled();
			
			BMessage newMessage(UPDATE_DESKTOP_MSG);
			
			const char *resolution = fResolutionMenu->FindMarked()->Label();
			
			//CheckModesByResolution(resolution);
			newMessage.AddString("resolution", resolution);
		
			PostMessage(&newMessage, fScreenDrawView);
			
			break;
		}
		
		case POP_COLORS_MSG:
		{
			CheckApplyEnabled();		
			break;
		}
		
		case POP_REFRESH_MSG:
		{
			CheckApplyEnabled();		
			break;
		}
		
		case POP_OTHER_REFRESH_MSG:
		{
			BRect frame(Frame());
			
			CheckApplyEnabled();
			int32 value = (int32)(fCustomRefresh * 10);		
			
			RefreshWindow *fRefreshWindow = new RefreshWindow(BRect((frame.left + 201.0),
				(frame.top + 34.0), (frame.left + 509.0),
				(frame.top + 169.0)), value);
			fRefreshWindow->Show();
			break;
		}
		
		case BUTTON_DEFAULTS_MSG:
		{
			char str[256];
			BMenuItem * item = 0;
			display_mode mode;
			mode.virtual_width = 640;
			mode.virtual_height = 480;
			mode_to_string(mode,str);
			if ((item = fResolutionMenu->FindItem(str)) != NULL) {
				item->SetMarked(true);
			} else {
				fResolutionMenu->ItemAt(0)->SetMarked(true);
			}
			colorspace_to_string(B_CMAP8,str);
			if ((item = fColorsMenu->FindItem(str)) != NULL) {
				item->SetMarked(true);
			} else {
				fColorsMenu->ItemAt(0)->SetMarked(true);
			}
			refresh_rate_to_string(60.0,str);
			if ((item = fRefreshMenu->FindItem(str)) != NULL) {
				item->SetMarked(true);
			} else {
				fRefreshMenu->ItemAt(0)->SetMarked(true);
			}
			CheckApplyEnabled();
			break;
		}
		
		case BUTTON_REVERT_MSG:
		case SET_INITIAL_MODE_MSG:
		{	
			fInitialResolution->SetMarked(true);
			fInitialColors->SetMarked(true);
			fInitialRefresh->SetMarked(true);
			
			CheckApplyEnabled();
			
			BMenuItem *other = fRefreshMenu->FindItem(POP_OTHER_REFRESH_MSG);
			
			if (fInitialRefresh == other) {
				BString string;			
				string << fInitialRefreshN;
				
				int32 point = string.FindFirst('.');
				string.Truncate(point + 2);
			
				string << " Hz/Other...";
			
				fRefreshMenu->FindItem(POP_OTHER_REFRESH_MSG)->SetLabel(string.String());
			
				point = string.FindFirst('/');
				string.Truncate(point);
			
				fRefreshMenu->Superitem()->SetLabel(string.String());
			}
			
			if (message->what == SET_INITIAL_MODE_MSG) {
				
				BScreen screen(B_MAIN_SCREEN_ID);
				if (!screen.IsValid())
					break;
						
				screen.SetMode(&fInitialMode, true);
			}
			break;
		}
			
		case BUTTON_APPLY_MSG:
		{	
			ApplyMode();
			break;
		}
				
		case SET_CUSTOM_REFRESH_MSG:
		{
			message->FindFloat("refresh", &fCustomRefresh);
		
			BMenuItem *other = fRefreshMenu->FindItem(POP_OTHER_REFRESH_MSG);	
				
			other->SetMarked(true);
			
			BString string;			
			string << fCustomRefresh;
			int32 point = string.FindFirst('.');
			string.Truncate(point + 2);
			
			string << " Hz/Other...";
			fRefreshMenu->FindItem(POP_OTHER_REFRESH_MSG)->SetLabel(string.String());
			
			point = string.FindFirst('/');
			string.Truncate(point);
			
			fRefreshMenu->Superitem()->SetLabel(string.String());
			
			if (fCustomRefresh != fInitialRefreshN) {
				fApplyButton->SetEnabled(true);
				fRevertButton->SetEnabled(true);
			}
		
			break;
		}
		
		case MAKE_INITIAL_MSG:
		{
			BScreen screen(B_MAIN_SCREEN_ID);
			if (!screen.IsValid())		
				break;
				
			display_mode mode;					
			screen.GetMode(&mode);
			
			if (fWorkspaceMenu->FindMarked() == fWorkspaceMenu->FindItem("All Workspaces")) {			
			
				int32 old = current_workspace();
				int32 totalWorkspaces = count_workspaces();
				fRevertButton->SetEnabled(false);
				
				for (int32 count = 0; count <= totalWorkspaces; count++) {
					if (count != old) {
						activate_workspace(count);
						screen.SetMode(&mode, true);
					}
				}
					
				activate_workspace(old);
						
			} else
				screen.SetMode(&mode, true);
		
			SetStateByMode();
			break;
		}
		
		default:
			BWindow::MessageReceived(message);		
			break;
	}
}


void
ScreenWindow::CheckApplyEnabled()
{
	if ((fResolutionMenu->FindMarked() != fInitialResolution)
			|| (fColorsMenu->FindMarked() != fInitialColors)
			|| (fRefreshMenu->FindMarked() != fInitialRefresh)) {
		fApplyButton->SetEnabled(true);
		fRevertButton->SetEnabled(true);
	} else {
		fApplyButton->SetEnabled(false);
		fRevertButton->SetEnabled(false);
	}			
			
}


void
ScreenWindow::CheckUpdateDisplayModes()
{
	uint32 c;
	
	// Add supported resolutions to the menu
	char mode[128];
	for (c = 0; c < fTotalModes; c++) {
		mode_to_string(fSupportedModes[c], mode);
		
		if (!fResolutionMenu->FindItem(mode))
			fResolutionMenu->AddItem(new BMenuItem(mode, new BMessage(POP_RESOLUTION_MSG)));
	}
	
	// Add supported colorspaces to the menu
	char colorSpace[128];
	for (c = 0; c < fTotalModes; c++) {
		colorspace_to_string(fSupportedModes[c].space, colorSpace);
		if (!fColorsMenu->FindItem(colorSpace))
			fColorsMenu->AddItem(new BMenuItem(colorSpace, new BMessage(POP_COLORS_MSG)));		
	}

	// Add supported refresh rates to the menu
	float min_refresh = 120, max_refresh = 0;
	for (c = 0; c < fTotalModes; c++) {
		display_mode mode = fSupportedModes[c];
		BScreen screen;
		uint32 low_clock, high_clock;
		screen.GetPixelClockLimits(&mode,&low_clock,&high_clock);
		mode.timing.pixel_clock = low_clock;
		min_refresh = min_c(min_refresh,get_refresh_rate(mode));
		mode.timing.pixel_clock = high_clock;
		max_refresh = max_c(max_refresh,get_refresh_rate(mode));
	}
	char refresh[128];
	float refreshes[] = { 56.0, 60.0, 70.0, 75.0, 85.0, 100.0, 120.0, 140.0 } ;
	for (uint i = 0 ; i < sizeof(refreshes)/sizeof(float) ; i++) {
		if ((min_refresh <= refreshes[i]) && (refreshes[i] <= max_refresh)) {
			refresh_rate_to_string(refreshes[i],refresh);
			if (!fRefreshMenu->FindItem(refresh))
				fRefreshMenu->AddItem(new BMenuItem(refresh, new BMessage(POP_REFRESH_MSG)));
		}
	}
	fMinRefresh = min_refresh;
	fMaxRefresh = max_refresh;
}


void
ScreenWindow::CheckModesByResolution(const char *res)
{
	BString resolution(res);
	
	bool found = false;
	uint32 c;
	int32 width = atoi(resolution.String());
	resolution.Remove(0, resolution.FindFirst('x') + 1);
	int32 height = atoi(resolution.String());
	
	for (c = 0; c < fTotalModes; c++) {
		if ((fSupportedModes[c].virtual_width == width) 
			&& (fSupportedModes[c].virtual_height == height)) {
			if (string_to_colorspace(fColorsMenu->FindMarked()->Label())
						== fSupportedModes[c].space) {
				found = true;
				break;
			}				
		}
	}
	
	if (!found) {
		fColorsMenu->ItemAt(fColorsMenu->IndexOf(fColorsMenu->FindMarked()) - 1)->SetMarked(true);	
		printf("not found\n");
	}
}

void
ScreenWindow::ApplyMode()
{
	BScreen screen(B_MAIN_SCREEN_ID);
	
	if (!screen.IsValid())
		return;
					
	display_mode requested_mode;
	screen.GetMode(&requested_mode); // start with current mode timings
						
	BString menuLabel = fResolutionMenu->FindMarked()->Label();
	string_to_mode(menuLabel.String(),&requested_mode);
	requested_mode.space = string_to_colorspace(fColorsMenu->FindMarked()->Label());
	requested_mode.h_display_start = 0;
	requested_mode.v_display_start = 0;

	display_mode * supported_mode = 0;
	display_mode * mismatch_mode = 0;
	for (uint32 c = 0; c < fTotalModes; c++) {			
		if ((fSupportedModes[c].virtual_width == requested_mode.virtual_width)
			&& (fSupportedModes[c].virtual_height == requested_mode.virtual_height)) {
			if (!mismatch_mode || (colorspace_to_bpp(mismatch_mode->space)
                                   < colorspace_to_bpp(fSupportedModes[c].space))) {
				mismatch_mode = &fSupportedModes[c];
            }
			if (fSupportedModes[c].space == requested_mode.space) {
				supported_mode = &fSupportedModes[c];
			}
		}
	}
	if (supported_mode) {
		requested_mode = *supported_mode;
	} else if (mismatch_mode) {
		display_mode proposed_mode = *mismatch_mode;
		BString string;
		char str[256];
		colorspace_to_string(requested_mode.space,str);
		string << str;
		mode_to_string(requested_mode,str);
		string << " is not supported at " << str << ".  ";
		string << "Use ";
		colorspace_to_string(proposed_mode.space,str);
		string << str;
		mode_to_string(proposed_mode,str);
		string << " at " << str << " instead?";
		BAlert * BadColorsAlert = 
			new BAlert("BadColorsAlert", string.String(),
			           "Okay", "Cancel", NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		int32 button = BadColorsAlert->Go();
		if (button == 1) {
			return;
		}
		requested_mode = proposed_mode;
	} else {
		display_mode proposed_mode = requested_mode;
		if (screen.ProposeMode(&proposed_mode,&requested_mode,&requested_mode) == B_ERROR) {
			BAlert * UnsupportedModeAlert = 
				new BAlert("UnsupportedModeAlert", "This mode is not supported.",
				           "Okay", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
			UnsupportedModeAlert->Go();
			return;
		}
		BString string;
		char str[256];
		colorspace_to_string(requested_mode.space,str);
		string << str;
		mode_to_string(requested_mode,str);
		string << " is not supported at " << str << ".  ";
		string << "Use ";
		colorspace_to_string(proposed_mode.space,str);
		string << str;
		mode_to_string(proposed_mode,str);
		string << " at " << str << " instead?";
		BAlert * BadModeAlert = 
			new BAlert("BadModeAlert", string.String(),
			           "Okay", "Cancel", NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		int32 button = BadModeAlert->Go();
		if (button == 1) {
			return;
		}
		requested_mode = proposed_mode;
	}

	float refresh;
	if (fRefreshMenu->FindMarked() == fRefreshMenu->FindItem(POP_OTHER_REFRESH_MSG))
		refresh = fCustomRefresh;
	else
		refresh = atof(fRefreshMenu->FindMarked()->Label());
	set_pixel_clock(&requested_mode,refresh);

	uint32 low_clock = 0, high_clock = 0;
	screen.GetPixelClockLimits(&requested_mode,&low_clock,&high_clock);
	if (requested_mode.timing.pixel_clock < low_clock) {
		display_mode low_mode = requested_mode;
		low_mode.timing.pixel_clock = low_clock;
		BString string;
		string << refresh << " Hz is too low.  ";
		string << "Use " << get_refresh_rate(low_mode) << " Hz instead?";
		BAlert * TooLowAlert = 
		   new BAlert("TooLowAlert", string.String(),
				      "Okay", "Cancel", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		int32 button = TooLowAlert->Go();
		if (button == 1) {
			return;
		}
		requested_mode.timing.pixel_clock = low_clock;
	}
	if (requested_mode.timing.pixel_clock > high_clock) {
		display_mode high_mode = requested_mode;
		high_mode.timing.pixel_clock = high_clock;
		BString string;
		string << refresh << " Hz is too high.";
		string << "Use " << get_refresh_rate(high_mode) << " Hz instead?";
		BAlert * TooHighAlert = 
		   new BAlert("TooHighAlert", string.String(),
				      "Okay", "Cancel", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		int32 button = TooHighAlert->Go();
		if (button == 1) {
			return;
		}
		requested_mode.timing.pixel_clock = high_clock;
	}	
	if (fWorkspaceMenu->FindMarked() == fWorkspaceMenu->FindItem("All Workspaces")) {			
		BAlert *WorkspacesAlert =
			new BAlert("WorkspacesAlert", "Change all workspaces?\nThis action cannot be reverted",
				"Okay", "Cancel", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
 				
		int32 button = WorkspacesAlert->Go();
		
		if (button == 1) {
			PostMessage(new BMessage(BUTTON_REVERT_MSG));
			return;
		}
	}
	screen.SetMode(&requested_mode);

	BRect rect(100.0, 100.0, 400.0, 193.0);
	
	rect.left = (screen.Frame().right / 2) - 150;
	rect.top = (screen.Frame().bottom / 2) - 42;
	rect.right = rect.left + 300.0;
	rect.bottom = rect.top + 93.0;
	
 	(new AlertWindow(rect))->Show();
							
	fApplyButton->SetEnabled(false);
}
