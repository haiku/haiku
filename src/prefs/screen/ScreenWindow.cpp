#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <InterfaceDefs.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <String.h>
#include <Screen.h>
#include <Window.h>

#include <math.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>

#include "ScreenWindow.h"
#include "ScreenDrawView.h"
#include "ScreenView.h"
#include "AlertWindow.h"
#include "Constants.h"
#include "Utility.h"

static const char*
colorspace_to_string(uint32 colorspace)
{
	switch (colorspace) {
		case B_RGB32:
			return "32 Bits/Pixel";	
		case B_RGB16:
			return"16 Bits/Pixel";
		case B_RGB15:
			return "15 Bits/Pixel";
		case B_CMAP8:
		default:		
			return "8 Bits/Pixel";
	}
}


static const char*
mode_to_string(display_mode mode)
{
	char string[128];
	sprintf(string, "%d x %d", mode.virtual_width, mode.virtual_height);
	
	return string;	
}

		 	
ScreenWindow::ScreenWindow(ScreenSettings *Settings)
	: BWindow(Settings->WindowFrame(), "Screen", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE, B_ALL_WORKSPACES)
{
	BRect frame(Bounds());
	BScreen screen(B_MAIN_SCREEN_ID);
	
	if (!screen.IsValid())
		;//debugger() ?
		
	screen.GetModeList(&fSupportedModes, &fTotalModes);
	
	fScreenView = new ScreenView(frame, "ScreenView");
	AddChild(fScreenView);
	
	fSettings = Settings;
	
	BRect ScreenBoxRect(11.0, 18.0, 153.0, 155.0);
	BRect ScreenDrawViewRect(20.0, 16.0, 122.0, 93.0);
	BRect ControlsBoxRect;
	BRect WorkspaceMenuRect;
	BRect WorkspaceCountMenuRect;
	BRect ControlMenuRect;
	BRect ButtonRect;
		
	BBox *screenBox = new BBox(ScreenBoxRect);
	screenBox->SetBorder(B_FANCY_BORDER);
		
	fScreenDrawView = new ScreenDrawView(ScreenDrawViewRect, "ScreenDrawView");
		
	fWorkspaceCountMenu = new BPopUpMenu("", true, true);
	
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
	
	WorkspaceCountMenuRect.Set(7.0, 107.0, 135.0, 127.0);
	
	fWorkspaceCountField = new BMenuField(WorkspaceCountMenuRect, "WorkspaceCountMenu", "Workspace count:", fWorkspaceCountMenu, true);
	
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
	
	WorkspaceMenuRect.Set(0.0, 0.0, 132.0, 18.0);
	
	fWorkspaceField = new BMenuField(WorkspaceMenuRect, "WorkspaceMenu", NULL, fWorkspaceMenu, true);
	
	ControlsBoxRect.Set(164.0, 7.0, 345.0, 155.0);
	
	BBox *controlsBox = new BBox(ControlsBoxRect);
	controlsBox->SetBorder(B_FANCY_BORDER);
	controlsBox->SetLabel(fWorkspaceField);
	
	ButtonRect.Set(88.0, 114.0, 200.0, 150.0);
	
	fApplyButton = new BButton(ButtonRect, "ApplyButton", "Apply", 
		new BMessage(BUTTON_APPLY_MSG));
	
	fApplyButton->AttachedToWindow();
	fApplyButton->ResizeToPreferred();
	fApplyButton->SetEnabled(false);
	
	controlsBox->AddChild(fApplyButton);
	
	fResolutionMenu = new BPopUpMenu("", true, true);	
	fColorsMenu = new BPopUpMenu("", true, true);
	
	CheckUpdateDisplayModes();
	
	ControlMenuRect.Set(33.0, 30.0, 171.0, 48.0);
	
	fResolutionField = new BMenuField(ControlMenuRect, "ResolutionMenu", "Resolution:", fResolutionMenu, true);
	
	marked = fResolutionMenu->ItemAt(0);
	marked->SetMarked(true);
	
	fResolutionField->SetDivider(55.0);
	
	controlsBox->AddChild(fResolutionField);
	
	ControlMenuRect.Set(50.0, 58.0, 171.0, 76.0);
	
	fColorsField = new BMenuField(ControlMenuRect, "ColorsMenu", "Colors:", fColorsMenu, true);
	
	marked = fColorsMenu->ItemAt(0);
	marked->SetMarked(true);
	
	fColorsField->SetDivider(38.0);
	
	controlsBox->AddChild(fColorsField);	
	
	// XXX: When we'll start to use OpenBeOS BScreen, we will be able to dynamically
	// build this menu using the available refresh rates, without limiting ourself
	// to 85 hz.
	fRefreshMenu = new BPopUpMenu("", true, true);
	fRefreshMenu->AddItem(new BMenuItem("56 Hz", new BMessage(POP_REFRESH_MSG)));
	fRefreshMenu->AddItem(new BMenuItem("60 Hz", new BMessage(POP_REFRESH_MSG)));
	fRefreshMenu->AddItem(new BMenuItem("70 Hz", new BMessage(POP_REFRESH_MSG)));
	fRefreshMenu->AddItem(new BMenuItem("75 Hz", new BMessage(POP_REFRESH_MSG)));
	fRefreshMenu->AddItem(new BMenuItem("85 Hz", new BMessage(POP_REFRESH_MSG)));
	fRefreshMenu->AddItem(new BMenuItem("100 Hz", new BMessage(POP_REFRESH_MSG)));
	
	fRefreshMenu->AddItem(new BMenuItem("Other...", new BMessage(POP_OTHER_REFRESH_MSG)));
		
	ControlMenuRect.Set(19.0, 86.0, 171.0, 104.0);
	
	fRefreshField = new BMenuField(ControlMenuRect, "RefreshMenu", "Refresh Rate:", fRefreshMenu, true);
	
	marked = fRefreshMenu->FindItem("60 Hz");
	marked->SetMarked(true);
	
	fRefreshField->SetDivider(69.0);
	
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
		
	display_mode mode;
	
	screen.GetMode(&mode);
	
	fInitialMode = mode;
	
	string.Truncate(0);
	
	string << (uint32)mode.virtual_width << " x " << (uint32)mode.virtual_height;

	marked = fResolutionMenu->FindItem(string.String());
	marked->SetMarked(true);
	
	fInitialResolution = marked;
	
	marked = fColorsMenu->FindItem(colorspace_to_string(mode.space));
		
	marked->SetMarked(true);
	fInitialColors = marked;
	
	string.Truncate(0);
	
	float total_size = mode.timing.h_total * mode.timing.v_total;
	
	fInitialRefreshN = round((mode.timing.pixel_clock * 1000) / total_size, 1);
	
	fCustomRefresh = fInitialRefreshN;
	
	string << fInitialRefreshN << " Hz";
	
	marked = fRefreshMenu->FindItem(string.String());
	
	if (marked != NULL)	{	
		marked->SetMarked(true);	
		fInitialRefresh = marked;	
	} else 	{		
		BMenuItem *other = fRefreshMenu->FindItem("Other...");	
		string.Truncate(0);
	
		string << fInitialRefreshN;
		
		int32 point = string.FindFirst('.');
		string.Truncate(point + 2);
		
		string << " Hz/Other...";
	
		other->SetLabel(string.String());
		other->SetMarked(true);
		
		point = string.FindFirst('/');
		string.Truncate(point);
		
		fRefreshMenu->Superitem()->SetLabel(string.String());
		
		fInitialRefresh = other;
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
			BMenuItem *Item = fWorkspaceCountMenu->FindMarked();
		
			set_workspace_count(fWorkspaceCountMenu->IndexOf(Item) + 1);
		
			break;
		}
		
		case POP_RESOLUTION_MSG:
		{
			CheckApplyEnabled();
		
			BMessage newMessage(UPDATE_DESKTOP_MSG);
			
			const char *resolution = fResolutionMenu->FindMarked()->Label();
			
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
			
			BMenuItem *other = fRefreshMenu->FindItem(POP_OTHER_REFRESH_MSG);
			
			BString string;
			
			string << other->Label();
			
			int32 point = string.FindFirst('z');
			string.Truncate(point + 1);
			
			fRefreshMenu->Superitem()->SetLabel(string.String());
			
			break;
		}
		
		case BUTTON_DEFAULTS_MSG:
		{	
			fResolutionMenu->ItemAt(0)->SetMarked(true);
			fColorsMenu->FindItem("8 Bits/Pixel")->SetMarked(true);
			fRefreshMenu->FindItem("60 Hz")->SetMarked(true);
			
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
			BScreen screen(B_MAIN_SCREEN_ID);
			
			if (!screen.IsValid())
				break;
							
			display_mode *mode = NULL;
			BString string;
			int32 height;
			int32 width;
			float refresh;
								
			BString menuLabel = fResolutionMenu->FindMarked()->Label();
			int32 space = menuLabel.FindFirst(' ');
			menuLabel.MoveInto(string, 0, space);
			menuLabel.Remove(0, 3);
					
			width = atoi(string.String());
			height = atoi(menuLabel.String());
			
			if (fRefreshMenu->FindMarked() == fRefreshMenu->FindItem(POP_OTHER_REFRESH_MSG))
				refresh = fCustomRefresh;
			else
				refresh = atof(fRefreshMenu->FindMarked()->Label());
			
			for (uint32 c = 0; c < fTotalModes; c++) {
				
				if ((fSupportedModes[c].virtual_width == width)
					&& (fSupportedModes[c].virtual_height == height))
					
				mode = &fSupportedModes[c];
			}
							
			mode->timing.pixel_clock = (uint32)((mode->timing.h_total * mode->timing.v_total) * refresh / 1000);					
			
			if (fColorsMenu->FindMarked() == fColorsMenu->FindItem("8 Bits/Pixel"))	
				mode->space = B_CMAP8;
			else if (fColorsMenu->FindMarked() == fColorsMenu->FindItem("15 Bits/Pixel"))		
				mode->space = B_RGB15;
			else if (fColorsMenu->FindMarked() == fColorsMenu->FindItem("16 Bits/Pixel"))	
				mode->space = B_RGB16;
			else if (fColorsMenu->FindMarked() == fColorsMenu->FindItem("32 Bits/Pixel"))
				mode->space = B_RGB32;
			
			mode->h_display_start = 0;
			mode->v_display_start = 0;

			if (fWorkspaceMenu->FindMarked() == fWorkspaceMenu->FindItem("All Workspaces")) {			
				BAlert *WorkspacesAlert =
					new BAlert("WorkspacesAlert", "Change all workspaces?\nThis action cannot be reverted",
						"Okay", "Cancel", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
 				
 				int32 button = WorkspacesAlert->Go();
 				
 				if (button == 1) {
 					PostMessage(new BMessage(BUTTON_REVERT_MSG));
 					break;
 				}
 					
			
				int32 old = current_workspace();
				int32 totalWorkspaces = count_workspaces();
				
				for (int32 count = 0; count < totalWorkspaces; count ++) {
					activate_workspace(count);
					screen.SetMode(mode, true);
				}
				
				activate_workspace(old);
			}
			else
				screen.SetMode(mode);
				
			BRect rect(100.0, 100.0, 400.0, 193.0);
			
			rect.left = (screen.Frame().right / 2) - 150;
			rect.top = (screen.Frame().bottom / 2) - 42;
			rect.right = rect.left + 300.0;
			rect.bottom = rect.top + 93.0;
			
		 	(new AlertWindow(rect))->Show();
									
			fApplyButton->SetEnabled(false);
			
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
			screen.SetMode(&mode, true);
		
			fInitialRefreshN = fCustomRefresh;
			fInitialResolution = fResolutionMenu->FindMarked();
			fInitialColors = fColorsMenu->FindMarked();
			fInitialRefresh = fRefreshMenu->FindMarked();
			fInitialMode = mode;
			
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
	for (c = 0; c < fTotalModes; c++) {
		const char *mode = mode_to_string(fSupportedModes[c]);
		
		if (!fResolutionMenu->FindItem(mode))
			fResolutionMenu->AddItem(new BMenuItem(mode, new BMessage(POP_RESOLUTION_MSG)));
	}
	
	// Add supported colorspaces to the menu
	for (c = 0; c < fTotalModes; c++) {
		const char *colorSpace = colorspace_to_string(fSupportedModes[c].space);
		
		if (!fColorsMenu->FindItem(colorSpace))
			fColorsMenu->AddItem(new BMenuItem(colorSpace, new BMessage(POP_COLORS_MSG)));		
	}
/* 
	XXX: BeOS's BScreen limits the refresh to 85 hz. I hope that OpenBeOS BScreen will remove
	this limitation. 
	
	for (c = 0; c < fTotalModes; c++) {
		display_mode *mode = &fSupportedModes[c];
		int32 refresh = (mode->timing.pixel_clock * 1000) / (mode->timing.h_total * mode->timing.v_total);
		printf("%d\n", refresh);	
	}
*/
}
