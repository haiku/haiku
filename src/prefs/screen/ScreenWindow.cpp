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

#include "ScreenWindow.h"
#include "ScreenDrawView.h"
#include "ScreenView.h"
#include "AlertWindow.h"
#include "Constants.h"
#include "Utility.h"
		 	
ScreenWindow::ScreenWindow(ScreenSettings *Settings)
	: BWindow(Settings->WindowFrame(), "Screen", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE, B_ALL_WORKSPACES)
{
	BRect frame(Bounds());
	BScreen screen(B_MAIN_SCREEN_ID);
	
	if (!screen.IsValid()) {
		//What could we do?
		//debugger() ?
	}
	screen.GetModeList(&fSupportedModes, &fTotalModes);
	
	fScreenView = new ScreenView(frame, "ScreenView");
	
	AddChild(fScreenView);
	
	fSettings = Settings;
	
	BRect ScreenBoxRect;
	BRect ScreenDrawViewRect;
	BRect ControlsBoxRect;
	BRect WorkspaceMenuRect;
	BRect WorkspaceCountMenuRect;
	BRect ControlMenuRect;
	BRect ButtonRect;
	
	ScreenBoxRect.Set(11.0, 18.0, 153.0, 155.0);
	
	fScreenBox = new BBox(ScreenBoxRect);
	fScreenBox->SetBorder(B_FANCY_BORDER);
	
	ScreenDrawViewRect.Set(20.0, 16.0, 122.0, 93.0);
	
	fScreenDrawView = new ScreenDrawView(ScreenDrawViewRect, "ScreenDrawView");
	
	BString String;
	
	String << count_workspaces();
	
	fWorkspaceCountMenu = new BPopUpMenu(String.String(), true, true);
	
	for (int32 Count = 1; Count <= 32; Count++)
	{
		String.Truncate(0);
		String << Count;
		fWorkspaceCountMenu->AddItem(new BMenuItem(String.String(), new BMessage(POP_WORKSPACE_CHANGED_MSG)));
	}
	
	String.Truncate(0);
	String << count_workspaces();
	
	BMenuItem *Marked = fWorkspaceCountMenu->FindItem(String.String());
	Marked->SetMarked(true);
	
	WorkspaceCountMenuRect.Set(7.0, 107.0, 135.0, 127.0);
	
	fWorkspaceCountField = new BMenuField(WorkspaceCountMenuRect, "WorkspaceCountMenu", "Workspace count:", fWorkspaceCountMenu, true);
	
	fWorkspaceCountField->SetDivider(91.0);
	
	fScreenBox->AddChild(fScreenDrawView);
	
	fScreenBox->AddChild(fWorkspaceCountField);
	
	fScreenView->AddChild(fScreenBox);
	
	fWorkspaceMenu = new BPopUpMenu("Current Workspace", true, true);
	fAllWorkspacesItem = new BMenuItem("All Workspaces", new BMessage(WORKSPACE_CHECK_MSG));
	fWorkspaceMenu->AddItem(fAllWorkspacesItem);
	fCurrentWorkspaceItem = new BMenuItem("Current Workspace", new BMessage(WORKSPACE_CHECK_MSG));
	fCurrentWorkspaceItem->SetMarked(true);
	fWorkspaceMenu->AddItem(fCurrentWorkspaceItem);
	
	WorkspaceMenuRect.Set(0.0, 0.0, 132.0, 18.0);
	
	fWorkspaceField = new BMenuField(WorkspaceMenuRect, "WorkspaceMenu", NULL, fWorkspaceMenu, true);
	
	ControlsBoxRect.Set(164.0, 7.0, 345.0, 155.0);
	
	fControlsBox = new BBox(ControlsBoxRect);
	fControlsBox->SetBorder(B_FANCY_BORDER);
	fControlsBox->SetLabel(fWorkspaceField);
	
	ButtonRect.Set(88.0, 114.0, 200.0, 150.0);
	
	fApplyButton = new BButton(ButtonRect, "ApplyButton", "Apply", 
	new BMessage(BUTTON_APPLY_MSG));
	
	fApplyButton->AttachedToWindow();
	fApplyButton->ResizeToPreferred();
	fApplyButton->SetEnabled(false);
	
	fControlsBox->AddChild(fApplyButton);
	
	fResolutionMenu = new BPopUpMenu("640 x 480", true, true);
	for (uint32 c = 0; c < fTotalModes; c++) {
		BString mode;
		mode << (int32)fSupportedModes[c].virtual_width << " x " << (int32)fSupportedModes[c].virtual_height;
		if (!fResolutionMenu->FindItem(mode.String()))
			fResolutionMenu->AddItem(new BMenuItem(mode.String(),
				new BMessage(POP_RESOLUTION_MSG)));
	}
	
	ControlMenuRect.Set(33.0, 30.0, 171.0, 48.0);
	
	fResolutionField = new BMenuField(ControlMenuRect, "ResolutionMenu", "Resolution:", fResolutionMenu, true);
	
	Marked = fResolutionMenu->FindItem("640 x 480");
	Marked->SetMarked(true);
	
	fResolutionField->SetDivider(55.0);
	
	fControlsBox->AddChild(fResolutionField);
	
	fColorsMenu = new BPopUpMenu("8 Bits/Pixel", true, true);
	fColorsMenu->AddItem(new BMenuItem("8 Bits/Pixel", new BMessage(POP_COLORS_MSG)));
	fColorsMenu->AddItem(new BMenuItem("15 Bits/Pixel", new BMessage(POP_COLORS_MSG)));
	fColorsMenu->AddItem(new BMenuItem("16 Bits/Pixel", new BMessage(POP_COLORS_MSG)));
	fColorsMenu->AddItem(new BMenuItem("32 Bits/Pixel", new BMessage(POP_COLORS_MSG)));
	
	ControlMenuRect.Set(50.0, 58.0, 171.0, 76.0);
	
	fColorsField = new BMenuField(ControlMenuRect, "ColorsMenu", "Colors:", fColorsMenu, true);
	
	Marked = fColorsMenu->FindItem("8 Bits/Pixel");
	Marked->SetMarked(true);
	
	fColorsField->SetDivider(38.0);
	
	fControlsBox->AddChild(fColorsField);	
	
	fRefreshMenu = new BPopUpMenu("60 Hz", true, true);
	fRefreshMenu->AddItem(new BMenuItem("56 Hz", new BMessage(POP_REFRESH_MSG)));
	fRefreshMenu->AddItem(new BMenuItem("60 Hz", new BMessage(POP_REFRESH_MSG)));
	fRefreshMenu->AddItem(new BMenuItem("70 Hz", new BMessage(POP_REFRESH_MSG)));
	fRefreshMenu->AddItem(new BMenuItem("72 Hz", new BMessage(POP_REFRESH_MSG)));
	fRefreshMenu->AddItem(new BMenuItem("75 Hz", new BMessage(POP_REFRESH_MSG)));
	fRefreshMenu->AddItem(new BMenuItem("Other...", new BMessage(POP_OTHER_REFRESH_MSG)));
	
	ControlMenuRect.Set(19.0, 86.0, 171.0, 104.0);
	
	fRefreshField = new BMenuField(ControlMenuRect, "RefreshMenu", "Refresh Rate:", fRefreshMenu, true);
	
	Marked = fRefreshMenu->FindItem("60 Hz");
	Marked->SetMarked(true);
	
	fRefreshField->SetDivider(69.0);
	
	fControlsBox->AddChild(fRefreshField);
	
	fScreenView->AddChild(fControlsBox);
	
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
	
	String.Truncate(0);
	
	String << (uint32)mode.virtual_width << " x " << (uint32)mode.virtual_height;

	Marked = fResolutionMenu->FindItem(String.String());
	Marked->SetMarked(true);
	
	fInitialResolution = Marked;
	
	if (mode.space == B_CMAP8){
		Marked = fColorsMenu->FindItem("8 Bits/Pixel");
		Marked->SetMarked(true);
	
	} else if (mode.space == B_RGB15) {
		Marked = fColorsMenu->FindItem("15 Bits/Pixel");
		Marked->SetMarked(true);
		
	} else if (mode.space == B_RGB16) {
		Marked = fColorsMenu->FindItem("16 Bits/Pixel");
		Marked->SetMarked(true);
		
	} else if (mode.space == B_RGB32) {
		Marked = fColorsMenu->FindItem("32 Bits/Pixel");
		Marked->SetMarked(true);
	}
	
	fInitialColors = Marked;
	
	String.Truncate(0);
	
	float total_size = mode.timing.h_total * mode.timing.v_total;
	
	fInitialRefreshN = round((mode.timing.pixel_clock * 1000) / total_size, 1);
	
	fCustomRefresh = fInitialRefreshN;
	
	String << fInitialRefreshN << " Hz";
	
	Marked = fRefreshMenu->FindItem(String.String());
	if (Marked != NULL)
	{
		Marked->SetMarked(true);
		
		fInitialRefresh = Marked;
	} else {
		BMenuItem *Other = fRefreshMenu->FindItem("Other...");
		
		String.Truncate(0);
	
		String << fInitialRefreshN;
		
		String.Truncate(4);
		
		String << " Hz/Other...";
	
		Other->SetLabel(String.String());
		Other->SetMarked(true);
		
		String.Truncate(7);
		
		fRefreshMenu->Superitem()->SetLabel(String.String());
		
		fInitialRefresh = Other;
	}
	
	Show();
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
ScreenWindow::CheckApplyEnabled()
{
	int equals = 0;

	if (fResolutionMenu->FindMarked() == fInitialResolution)
		equals++;
	else
		equals--;
	
	if (fColorsMenu->FindMarked() == fInitialColors)
		equals++;
	else
		equals--;
	
	if (fRefreshMenu->FindMarked() == fInitialRefresh)
		equals++;
	else
		equals--;
		
	if(equals != 3)
	{
		fApplyButton->SetEnabled(true);
		fRevertButton->SetEnabled(true);
	} else {
		fApplyButton->SetEnabled(false);
		fRevertButton->SetEnabled(false);
	}
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
		
			BMessage *Message = new BMessage(UPDATE_DESKTOP_MSG);
			
			const char *Resolution = fResolutionMenu->FindMarked()->Label();
			
			Message->AddString("resolution", Resolution);
		
			PostMessage(Message, fScreenDrawView);
			
			if (fResolutionMenu->FindMarked() == fInitialResolution)
			
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
			CheckApplyEnabled();
			int32 value = (int32)(fCustomRefresh * 10);		

			fRefreshWindow = new RefreshWindow(BRect((Frame().left + 201.0),
				(Frame().top + 34.0), (Frame().left + 509.0),
				(Frame().top + 169.0)), value);
		
			BMenuItem *Other = fRefreshMenu->FindItem(POP_OTHER_REFRESH_MSG);
			
			BString String;
			
			String << Other->Label();
			
			String.Truncate(7);
			
			fRefreshMenu->Superitem()->SetLabel(String.String());
			
			break;
		}
		
		case BUTTON_DEFAULTS_MSG:
		{	
			fResolutionMenu->FindItem("640 x 480")->SetMarked(true);
			fColorsMenu->FindItem("8 Bits/Pixel")->SetMarked(true);
			fRefreshMenu->FindItem("60 Hz")->SetMarked(true);
			
			CheckApplyEnabled();
			
			break;
		}
		
		case BUTTON_REVERT_MSG:
		{	
			fInitialResolution->SetMarked(true);
			fInitialColors->SetMarked(true);
			fInitialRefresh->SetMarked(true);
			
			CheckApplyEnabled();
			
			BMenuItem *Other = fRefreshMenu->FindItem(POP_OTHER_REFRESH_MSG);
			
			if (fInitialRefresh == Other)
			{
				BString string;
				
				string << fInitialRefreshN;
			
				string.Truncate(4);
			
				string << " Hz/Other...";
			
				fRefreshMenu->FindItem(POP_OTHER_REFRESH_MSG)->SetLabel(string.String());
			
				string.Truncate(7);
			
				fRefreshMenu->Superitem()->SetLabel(string.String());
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
			refresh = atof(fRefreshMenu->FindMarked()->Label());
			
			for(uint32 c = 0; c < fTotalModes; c++)
			{
				if ((fSupportedModes[c].virtual_width == width)
					&& (fSupportedModes[c].virtual_height == height))
					
				mode = &fSupportedModes[c];
			}
							
			if (fRefreshMenu->FindMarked() == fRefreshMenu->FindItem(POP_OTHER_REFRESH_MSG))
				mode->timing.pixel_clock = (uint32)((mode->timing.h_total * mode->timing.v_total) * fCustomRefresh / 1000);
			else
				mode->timing.pixel_clock = (uint32)((mode->timing.h_total * mode->timing.v_total) * refresh / 1000);
			
			
			
			if (fColorsMenu->FindMarked() == fColorsMenu->FindItem("8 Bits/Pixel"))
			{	
				mode->space = B_CMAP8;
			}
			else if (fColorsMenu->FindMarked() == fColorsMenu->FindItem("15 Bits/Pixel"))
			{	
				mode->space = B_RGB15;
			}
			else if (fColorsMenu->FindMarked() == fColorsMenu->FindItem("16 Bits/Pixel"))
			{	
				mode->space = B_RGB16;
			}
			else if (fColorsMenu->FindMarked() == fColorsMenu->FindItem("32 Bits/Pixel"))
			{	
				mode->space = B_RGB32;
			}
			
			mode->h_display_start = 0;
			mode->v_display_start = 0;

			if (fWorkspaceMenu->FindMarked() == fWorkspaceMenu->FindItem("All Workspaces"))
			{
				int32 button;
			
				BAlert *WorkspacesAlert = new BAlert("WorkspacesAlert", "Change all workspaces? 
This action cannot be reverted", "Okay", "Cancel", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
 				button = WorkspacesAlert->Go();
 				
 				if (button == 1)
 					break;
			
				int32 old = current_workspace();
				int32 totalWorkspaces = count_workspaces();
				
				for (int32 count = 0; count < totalWorkspaces; count ++)
				{
					activate_workspace(count);
					screen.SetMode(mode);
				}
				
				activate_workspace(old);
			}
			else
				screen.SetMode(mode);
				
			BRect Rect(100.0, 100.0, 400.0, 193.0);
			
			Rect.left = (screen.Frame().right / 2) - 150;
			Rect.top = (screen.Frame().bottom / 2) - 42;
			Rect.right = Rect.left + 300.0;
			Rect.bottom = Rect.top + 93.0;
			
			new AlertWindow(Rect);
			
			fApplyButton->SetEnabled(false);
			
			break;
		}
		
		case SET_INITIAL_MODE_MSG:
		{
			BMenuItem *Other = fRefreshMenu->FindItem(POP_OTHER_REFRESH_MSG);
		
			if (fInitialRefresh == Other)
			{	
				Other->SetMarked(true);
				
				BString String;
				
				String << fInitialRefreshN;
			
				String.Truncate(4);
			
				String << " Hz/Other...";
			
				fRefreshMenu->FindItem(POP_OTHER_REFRESH_MSG)->SetLabel(String.String());
			
				String.Truncate(7);
			
				fRefreshMenu->Superitem()->SetLabel(String.String());
			}
			BScreen screen(B_MAIN_SCREEN_ID);			
			screen.SetMode(&fInitialMode);
		
			break;
		}
		
		case SET_CUSTOM_REFRESH_MSG:
		{
			message->FindFloat("refresh", &fCustomRefresh);
		
			BMenuItem *Other = fRefreshMenu->FindItem(POP_OTHER_REFRESH_MSG);	
				
			Other->SetMarked(true);
				
			BString String;
				
			String << fCustomRefresh;
			
			String.Truncate(4);
			
			String << " Hz/Other...";
			
			fRefreshMenu->FindItem(POP_OTHER_REFRESH_MSG)->SetLabel(String.String());
			
			String.Truncate(7);
			
			fRefreshMenu->Superitem()->SetLabel(String.String());
			
			if (fCustomRefresh != fInitialRefreshN)
			{
				fApplyButton->SetEnabled(true);
				fRevertButton->SetEnabled(true);
			}
		
			break;
		}
		
		case MAKE_INITIAL_MSG:
		{
			BScreen screen(B_MAIN_SCREEN_ID);		
			display_mode mode;		
			
			screen.GetMode(&mode);
		
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
