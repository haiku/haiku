// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:        TMWindow.cpp
//  Author:      Jérôme Duval
//  Description: Keyboard input server addon
//  Created :    October 13, 2004
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include "TMWindow.h"
#include "TMListItem.h"
#include "KeyboardInputDevice.h"
#include <Message.h>
#include <ScrollView.h>
#include <Screen.h>

const uint32 TM_CANCEL = 'TMca';
const uint32 TM_FORCE_REBOOT = 'TMfr';
const uint32 TM_KILL_APPLICATION = 'TMka';
const uint32 TM_SELECTED_TEAM = 'TMst';

TMWindow::TMWindow()
	: BWindow(BRect(0,0,350,300), "Team Monitor", 
		B_TITLED_WINDOW_LOOK, B_MODAL_ALL_WINDOW_FEEL, 
		B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE| B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS),
		fQuitting(false)
{
	Lock();

	BRect rect = Bounds();
	
	fBackground = new TMBox(rect, "background", B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW, B_NO_BORDER);
	AddChild(fBackground);
	
	rect = Bounds();
	rect.right -= 10;
	rect.left = rect.right - 75;
	rect.bottom -= 14;
	rect.top = rect.bottom - 20;
	
	BButton *cancel = new BButton(rect, "cancel", "Cancel", 
		new BMessage(TM_CANCEL), B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
	fBackground->AddChild(cancel);
	SetDefaultButton(cancel);
	
	rect.left = 10;
	rect.right = rect.left + 83;
	
	BButton *forceReboot = new BButton(rect, "force", "Force Reboot", 
		new BMessage(TM_FORCE_REBOOT), B_FOLLOW_LEFT|B_FOLLOW_BOTTOM);
	fBackground->AddChild(forceReboot);
	
	rect.top -= 97;
	rect.bottom = rect.top + 20;
	rect.right += 1;
		
	fKillApp = new BButton(rect, "kill", "Kill Application", 
		new BMessage(TM_KILL_APPLICATION), B_FOLLOW_LEFT|B_FOLLOW_BOTTOM);
	fBackground->AddChild(fKillApp);
	fKillApp->SetEnabled(false);
	
	BRect	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	BPoint 	pt;
	pt.x = screenFrame.Width()/2 - Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - Bounds().Height()/2;

	if (screenFrame.Contains(pt))
		MoveTo(pt);

	Unlock();
}


TMWindow::~TMWindow()
{


}


void
TMWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case TM_KILL_APPLICATION: {
				TMListItem *item = (TMListItem*)fBackground->fListView->ItemAt(
					fBackground->fListView->CurrentSelection());
				kill_team(item->GetInfo()->team);
				fKillApp->SetEnabled(false);
			}
			break;
		case TM_SELECTED_TEAM:
			fKillApp->SetEnabled(true);
			break;
		case TM_CANCEL:
			Disable();
			break;
		case SYSTEM_SHUTTING_DOWN:
			fQuitting = true;
			break;
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


bool
TMWindow::QuitRequested()
{
	Disable();
	return fQuitting;
}


void
TMWindow::Enable()
{
	SetPulseRate(1000000);
	
	if (IsHidden())
		Show();
}


void
TMWindow::Disable()
{
	SetPulseRate(0);
	Hide();
}


TMBox::TMBox(BRect bounds, const char* name, uint32 resizeFlags,
		uint32 flags, border_style border)
	: BBox(bounds, name, resizeFlags, flags | B_PULSE_NEEDED, border)
{

	BRect rect = Bounds();
	rect.InsetBy(12, 11);
	rect.right -= B_V_SCROLL_BAR_WIDTH;
	rect.bottom = rect.top + 146;
	
	fListView = new BListView(rect, "teams", B_SINGLE_SELECTION_LIST, 
		B_FOLLOW_ALL);
	fListView->SetSelectionMessage(new BMessage(TM_SELECTED_TEAM));
	
	BScrollView *sv = new BScrollView("scroll_teams", fListView, 
		B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP_BOTTOM, 0, false, true, B_FANCY_BORDER);
	
	AddChild(sv);
	
}


void
TMBox::Pulse()
{
	CALLED();
	int32 cookie = 0;
	team_info info;
	
	for (int32 i=0; i<fListView->CountItems(); i++) {
		TMListItem *item = (TMListItem*)fListView->ItemAt(i);
		item->fFound = false;
	}
	
	while (get_next_team_info(&cookie, &info) == B_OK) {
		if (info.team <=16)
			continue;
	
		bool found = false;
		for (int32 i=0; i<fListView->CountItems(); i++) {
			TMListItem *item = (TMListItem*)fListView->ItemAt(i);
			if (item->GetInfo()->team == info.team) {
				item->fFound = true;
				found = true;
			}
		}
		
		if (!found) {
			TMListItem *item = new TMListItem(info);
			fListView->AddItem(item, 0);
			item->fFound = true;
		}
	}	

	for (int32 i=fListView->CountItems()-1; i>=0; i--) {
		TMListItem *item = (TMListItem*)fListView->ItemAt(i);
		if (!item->fFound)
			delete fListView->RemoveItem(i);
	}

	fListView->Invalidate();
}
