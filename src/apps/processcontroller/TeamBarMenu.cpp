/*

	TeamBarMenu.cpp

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

#include <Roster.h>
#include <Bitmap.h>
#include <stdlib.h>
#include "TeamBarMenu.h"
#include "ThreadBarMenu.h"
#include "TeamBarMenuItem.h"
#include "NoiseBarMenuItem.h"
#include "PCView.h"
#include <Window.h>

#define EXTRA 10

TeamBarMenu::TeamBarMenu (const char *title, infosPack *infos, int32 teamCount)
	:BMenu (title), fTeamCount (teamCount+EXTRA), fFirstShow (true)
{
	SetFlags (Flags () | B_PULSE_NEEDED);
	fTeamList = (team_id*) malloc (sizeof (team_id) * fTeamCount);
	int	k;
	for (k = 0; k < teamCount; k++)
		fTeamList[k] = infos[k].tminfo.team;
	while (k < fTeamCount)
		fTeamList[k++] = -1;
	fRecycleCount = EXTRA;
	fRecycleList = (TRecycleItem*) malloc (sizeof (TRecycleItem) * fRecycleCount);
	SetFont (be_plain_font);
	gCurrentThreadBarMenu = NULL;
	fLastTotalTime = system_time ();
	AddItem (new NoiseBarMenuItem ());
}

TeamBarMenu::~TeamBarMenu ()
{
	gCurrentThreadBarMenu = NULL;
	free (fTeamList);
	free (fRecycleList);
}

void TeamBarMenu::Draw (BRect updateRect)
{
	BMenu::Draw (updateRect);
	if (fFirstShow)
	{
		Pulse ();
		fFirstShow = false;
	}
}

void TeamBarMenu::Pulse ()
{
//	Window ()->SetPulseRate (50000);
	Window ()->BeginViewTransaction ();

	// create the list of items to remove, for their team is gone. Update the old teams.
	int lastRecycle = 0;
	int firstRecycle = 0;
	int	k;
	TeamBarMenuItem *item;
	double			total = 0;
	for (k = 1; (item = (TeamBarMenuItem*) ItemAt(k)) != NULL; k++) {
		item->BarUpdate();
		if (item->fKernel < 0) {
			if (lastRecycle == fRecycleCount) {
				fRecycleCount += EXTRA;
				fRecycleList = (TRecycleItem*) realloc(fRecycleList, sizeof(TRecycleItem)*fRecycleCount);
			}
			fRecycleList[lastRecycle].index = k;
			fRecycleList[lastRecycle++].item = item;
		} else {
			if (lastRecycle > 0) {
				RemoveItems(fRecycleList[0].index, lastRecycle, true);
				k -= lastRecycle;
				lastRecycle = 0;
			}
			total += item->fUser+item->fKernel;
		}
	}

	// Look new teams that have appeared. Create an item for them, or recycle from the list.
	int32		cookie = 0;
	infosPack	infos;
	item = NULL;
	while (get_next_team_info(&cookie, &infos.tminfo) == B_OK) {
		int	j = 0;
		while (j < fTeamCount && infos.tminfo.team != fTeamList[j])
			j++;
		if (infos.tminfo.team != fTeamList[j]) {
			// new team
			team_info	info;
			j = 0;
			while (j < fTeamCount && fTeamList[j] != -1)
				if (get_team_info(fTeamList[j], &info) != B_OK)
					fTeamList[j] = -1;
				else
					j++;
			if (j == fTeamCount) {
				fTeamCount +=  10;
				fTeamList = (team_id*) realloc(fTeamList, sizeof(team_id)*fTeamCount);
			}
			fTeamList[j] = infos.tminfo.team;
			if (!get_team_name_and_icon(infos, true)) {
				// the team is already gone!
				delete infos.tmicon;
				fTeamList[j] = -1;
			} else {
				if (!item && firstRecycle < lastRecycle) {
					item = fRecycleList[firstRecycle++].item;
				}
				if (item) {
					item->Reset(infos.tmname, infos.tminfo.team, infos.tmicon, true);
				} else {
					BMessage* kill_team = new BMessage('KlTm');
					kill_team->AddInt32("team", infos.tminfo.team);
					item = new TeamBarMenuItem(new ThreadBarMenu(infos.tmname, infos.tminfo.team, infos.tminfo.thread_count),
						kill_team, infos.tminfo.team, infos.tmicon, true);
					item->SetTarget(gPCView);
					AddItem(item);
					item->BarUpdate();
				}
				if (item->fKernel >= 0) {
					total += item->fUser+item->fKernel;
					item = NULL;
				} else {
					fTeamList[j] = -1;
				}
			}
		}
	}
	if (item) {
		RemoveItem(item);
		delete item;
	}		
	
	// Delete the items that haven't been recycled.
	if (firstRecycle < lastRecycle)
		RemoveItems(IndexOf(fRecycleList[firstRecycle].item), lastRecycle-firstRecycle, true);

	total /= gCPUcount;
	total = 1-total;

	fLastTotalTime = system_time ();
	NoiseBarMenuItem	*noiseItem;
	if ((noiseItem = (NoiseBarMenuItem*) ItemAt(0)) != NULL) {
		noiseItem->fBusyWaiting = 0;
		noiseItem->fLost = (total >= 0 ? total : 0);
		noiseItem->DrawBar(false);
	}
	
//	status_t	st;
//	if (gCurrentThreadBarMenu && (st = gCurrentThreadBarMenu->LockLooperWithTimeout(50000)) == B_OK) {
	if (gCurrentThreadBarMenu && gCurrentThreadBarMenu->LockLooperWithTimeout(25000) == B_OK) {
		gCurrentThreadBarMenu->Window()->BeginViewTransaction();
		gCurrentThreadBarMenu->Update();
		gCurrentThreadBarMenu->Window()->EndViewTransaction();
		gCurrentThreadBarMenu->Window()->Flush();
		gCurrentThreadBarMenu->UnlockLooper();
	}
//	else
//		if (st == B_TIMED_OUT)
//			beep();
	Window()->EndViewTransaction();
	Window()->Flush();
}
