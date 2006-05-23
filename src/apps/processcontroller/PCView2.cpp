/*
	
	PCView2.cpp
	
	ProcessController
	© 2000, Georges-Edouard Berenger, All Rights Reserved.
	Copyright (C) 2004 beunited.org 

	This library is free software; you can redistribute it and/or 
	modify it under the terms of the GNU Lesser General Public 
	License as published by the Free Software Foundation; either 
	version 2.1 of the License, or (at your option) any later version. 

	This library is distributed in the hope that it will be useful, 
	but WITHOUT ANY WARRANTY; without even the implied warranty of 
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
	Lesser General Public License for more details. 

	You should have received a copy of the GNU Lesser General Public 
	License along with this library; if not, write to the Free Software 
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA	
	
*/

#include "PCView.h"
#include "QuitMenu.h"
#include "IconMenuItem.h"
#include "TeamBarMenu.h"
#include "TeamBarMenuItem.h"
#include "ThreadBarMenu.h"
#include "MemoryBarMenu.h"
#include "MemoryBarMenuItem.h"
#include "PCWorld.h"
#include "Preferences.h"
#include "PCUtils.h"
#include "AutoIcon.h"
#include <Screen.h>
#include <Bitmap.h>
#include <Roster.h>
#include <PopUpMenu.h>
#include <Deskbar.h>
#include <stdio.h>

#define addtopbottom(x) if (top) popup->AddItem(x); else popup->AddItem(x, 0)

long thread_popup(void *arg);

int32			gPopupFlag = 0;
thread_id		gPopupThreadID = 0;

typedef struct {
	BPoint				where;
	BRect				clickToOpenRect;
	bool				top;
} Tpopup_param;

void ProcessController::MouseDown(BPoint where)
{
	if (atomic_add (&gPopupFlag, 1) > 0) {
		atomic_add (&gPopupFlag, -1);
		return;
	}
	Tpopup_param*	param = new Tpopup_param;
	ConvertToScreen(&where);
	param->where = where;
	param->clickToOpenRect = Frame ();
	ConvertToScreen (&param->clickToOpenRect);
	param->top = where.y < BScreen(this->Window()).Frame().bottom-50;
	gPopupThreadID = spawn_thread(thread_popup, "Popup holder thread", B_URGENT_DISPLAY_PRIORITY, param);
	resume_thread(gPopupThreadID);
}

long thread_popup(void *arg)
{
	Tpopup_param*	param = (Tpopup_param*) arg;
	system_info		sinfo;
	int32			mcookie, hcookie;
	long			m, h;
	BMenuItem		*item;
	bool			top = param->top;
	
	get_system_info(&sinfo);
	infosPack		*infos = new infosPack[sinfo.used_teams];
	for (m = 0, mcookie = 0; m < sinfo.used_teams; m++) {
		infos[m].tmicon = NULL;
		infos[m].tmname[0] = 0;
		infos[m].thinfo = NULL;
		if (get_next_team_info(&mcookie, &infos[m].tminfo) == B_OK) {
			infos[m].thinfo = new thread_info[infos[m].tminfo.thread_count];
			for (h = 0, hcookie = 0; h < infos[m].tminfo.thread_count; h++)
				if (get_next_thread_info(infos[m].tminfo.team, &hcookie, &infos[m].thinfo[h]) != B_OK)
					infos[m].thinfo[h].thread = -1;
			get_team_name_and_icon(infos[m], true);
		} else {
			sinfo.used_teams = m;
			infos[m].tminfo.team = -1;
		}
	}

	BPopUpMenu* popup = new BPopUpMenu("Global Popup", false, false);
	popup->SetFont(be_plain_font);

	// Quit section
	BMenu* QuitPopup = new QuitMenu ("Quit an Application", infos, sinfo.used_teams);
	QuitPopup->SetFont (be_plain_font);
	popup->AddItem (QuitPopup);

	//Memory Usage section
	MemoryBarMenu* MemoryPopup = new MemoryBarMenu ("Spy Memory Usage", infos, &sinfo);
	int commitedMemory = int (sinfo.used_pages * B_PAGE_SIZE / 1024);
	for (m = 0; m < sinfo.used_teams; m++)
		if (infos[m].tminfo.team >= 0)
		{
			MemoryBarMenuItem* memoryItem = new MemoryBarMenuItem (infos[m].tmname, infos[m].tminfo.team, infos[m].tmicon, false, NULL);
			MemoryPopup->AddItem (memoryItem);
			memoryItem->UpdateSituation (commitedMemory);
		}
	addtopbottom (MemoryPopup);

	//CPU Load section
	TeamBarMenu* CPUPopup = new TeamBarMenu ("Kill, Debug, or Change Priority", infos, sinfo.used_teams);
	for (m = 0; m < sinfo.used_teams; m++) {	
		if (infos[m].tminfo.team >= 0) {
			ThreadBarMenu* TeamPopup = new ThreadBarMenu (infos[m].tmname, infos[m].tminfo.team, infos[m].tminfo.thread_count);
			BMessage* kill_team = new BMessage ('KlTm');
			kill_team->AddInt32 ("team", infos[m].tminfo.team);
			TeamBarMenuItem* item = new TeamBarMenuItem (TeamPopup, kill_team, infos[m].tminfo.team, infos[m].tmicon, false);
			item->SetTarget (gPCView);
			CPUPopup->AddItem (item);
		}
	}
	addtopbottom (CPUPopup);
	addtopbottom (new BSeparatorItem ());

	// CPU on/off section
	if (gCPUcount > 1)
	{
		for (int i = 0; i < gCPUcount; i++)
		{
			char item_name[32];
			sprintf (item_name, "Processor %d", i + 1);
			BMessage* m = new BMessage ('CPU ');
			m->AddInt32 ("cpu", i);
			item = new IconMenuItem (gPCView->fProcessorIcon, item_name, m);
			if (_kget_cpu_state_ (i))
				item->SetMarked (true);
			item->SetTarget(gPCView);
			addtopbottom(item);
		}
		addtopbottom (new BSeparatorItem ());
	}

	if (!be_roster->IsRunning(kTrackerSig)) {
		item = new IconMenuItem(gPCView->fTrackerIcon, "Launch Tracker", new BMessage('Trac'));
		item->SetTarget(gPCView);
		addtopbottom(item);
	}
	if (!be_roster->IsRunning(kDeskbarSig)) {
		item = new IconMenuItem(gPCView->fDeskbarIcon, "Launch Deskbar", new BMessage('Dbar'));
		item->SetTarget(gPCView);
		addtopbottom(item);
	}

	item = new IconMenuItem(gPCView->fTerminalIcon, "New Terminal", new BMessage('Term'));
	item->SetTarget(gPCView);
	addtopbottom(item);

	addtopbottom(new BSeparatorItem());

	if (be_roster->IsRunning(kDeskbarSig)) {
		item = new BMenuItem("Live in the Deskbar", new BMessage ('AlDb'));
		BDeskbar db;
		item->SetMarked (gInDeskbar || db.HasItem (kDeskbarItemName));
		item->SetTarget (gPCView);
		addtopbottom (item);
	}

	item = new BMenuItem("Use Pulse's Settings for Colors", new BMessage ('Colo'));
	item->SetTarget (gPCView);
	item->SetMarked (gMimicPulse);
	addtopbottom (item);

	item = new BMenuItem("Read Documentation", new BMessage ('Docu'));
	item->SetTarget (gPCView);
	addtopbottom (item);

	addtopbottom (new BSeparatorItem ());

	item = new IconMenuItem (gPCView->fProcessControllerIcon, "About ProcessController", new BMessage(B_ABOUT_REQUESTED));
	item->SetTarget(gPCView);
	addtopbottom(item);
	
	param->where.x -= 5;
	param->where.y -= 8;
	popup->Go(param->where, true, true, param->clickToOpenRect);
	
	delete popup;
	for (m = 0; m < sinfo.used_teams; m++) {
		if (infos[m].tminfo.team >= 0) {
			delete[] infos[m].thinfo;
			delete infos[m].tmicon;
		}
	}
	delete[] infos;
	delete param;
	atomic_add (&gPopupFlag, -1);
	gPopupThreadID = 0;

	return B_OK;
}
