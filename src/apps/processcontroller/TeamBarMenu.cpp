/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "TeamBarMenu.h"
#include "ThreadBarMenu.h"
#include "TeamBarMenuItem.h"
#include "NoiseBarMenuItem.h"
#include "ProcessController.h"

#include <Bitmap.h>
#include <Roster.h>
#include <Window.h>

#include <stdlib.h>


#define EXTRA 10


TeamBarMenu::TeamBarMenu(const char* title, info_pack* infos, int32 teamCount)
	: BMenu(title),
	fTeamCount(teamCount+EXTRA),
	fFirstShow(true)
{
	SetFlags(Flags() | B_PULSE_NEEDED);
	fTeamList = (team_id*)malloc(sizeof(team_id) * fTeamCount);
	int	k;
	for (k = 0; k < teamCount; k++) {
		fTeamList[k] = infos[k].team_info.team;
	}
	while (k < fTeamCount) {
		fTeamList[k++] = -1;
	}
	fRecycleCount = EXTRA;
	fRecycleList = (TRecycleItem*)malloc(sizeof(TRecycleItem) * fRecycleCount);
	SetFont(be_plain_font);
	gCurrentThreadBarMenu = NULL;
	fLastTotalTime = system_time();
	AddItem(new NoiseBarMenuItem());
}


TeamBarMenu::~TeamBarMenu()
{
	gCurrentThreadBarMenu = NULL;
	free(fTeamList);
	free(fRecycleList);
}


void
TeamBarMenu::Draw(BRect updateRect)
{
	BMenu::Draw (updateRect);
	if (fFirstShow) {
		Pulse();
		fFirstShow = false;
	}
}


void
TeamBarMenu::Pulse()
{
	Window()->BeginViewTransaction();

	// create the list of items to remove, for their team is gone. Update the old teams.
	int lastRecycle = 0;
	int firstRecycle = 0;
	int	k;
	TeamBarMenuItem *item;
	double total = 0;
	for (k = 1; (item = (TeamBarMenuItem*)ItemAt(k)) != NULL; k++) {
		item->BarUpdate();
		if (item->fKernel < 0) {
			if (lastRecycle == fRecycleCount) {
				fRecycleCount += EXTRA;
				fRecycleList = (TRecycleItem*)realloc(fRecycleList,
					sizeof(TRecycleItem)*fRecycleCount);
			}
			fRecycleList[lastRecycle].index = k;
			fRecycleList[lastRecycle++].item = item;
		} else {
			if (lastRecycle > 0) {
				RemoveItems(fRecycleList[0].index, lastRecycle, true);
				k -= lastRecycle;
				lastRecycle = 0;
			}
			total += item->fUser + item->fKernel;
		}
	}

	// Look new teams that have appeared. Create an item for them, or recycle from the list.
	int32 cookie = 0;
	info_pack infos;
	item = NULL;
	while (get_next_team_info(&cookie, &infos.team_info) == B_OK) {
		int	j = 0;
		while (j < fTeamCount && infos.team_info.team != fTeamList[j])
			j++;
		if (infos.team_info.team != fTeamList[j]) {
			// new team
			team_info info;
			j = 0;
			while (j < fTeamCount && fTeamList[j] != -1)
				if (get_team_info(fTeamList[j], &info) != B_OK)
					fTeamList[j] = -1;
				else
					j++;
			if (j == fTeamCount) {
				fTeamCount +=  10;
				fTeamList = (team_id*)realloc(fTeamList, sizeof(team_id) * fTeamCount);
			}
			fTeamList[j] = infos.team_info.team;
			if (!get_team_name_and_icon(infos, true)) {
				// the team is already gone!
				delete infos.team_icon;
				fTeamList[j] = -1;
			} else {
				if (!item && firstRecycle < lastRecycle) {
					item = fRecycleList[firstRecycle++].item;
				}
				if (item) {
					item->Reset(infos.team_name, infos.team_info.team, infos.team_icon, true);
				} else {
					BMessage* kill_team = new BMessage('KlTm');
					kill_team->AddInt32("team", infos.team_info.team);
					item = new TeamBarMenuItem(new ThreadBarMenu(infos.team_name,
						infos.team_info.team, infos.team_info.thread_count),
						kill_team, infos.team_info.team, infos.team_icon, true);
					item->SetTarget(gPCView);
					AddItem(item);
					item->BarUpdate();
				}
				if (item->fKernel >= 0) {
					total += item->fUser + item->fKernel;
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
		RemoveItems(IndexOf(fRecycleList[firstRecycle].item), lastRecycle - firstRecycle, true);

	total /= gCPUcount;
	total = 1 - total;

	fLastTotalTime = system_time();
	NoiseBarMenuItem* noiseItem;
	if ((noiseItem = (NoiseBarMenuItem*)ItemAt(0)) != NULL) {
		noiseItem->SetBusyWaiting(0);
		if (total >= 0)
			noiseItem->SetLost(total);
		else
			noiseItem->SetLost(0);
		noiseItem->DrawBar(false);
	}

	if (gCurrentThreadBarMenu && gCurrentThreadBarMenu->LockLooperWithTimeout(25000) == B_OK) {
		gCurrentThreadBarMenu->Window()->BeginViewTransaction();
		gCurrentThreadBarMenu->Update();
		gCurrentThreadBarMenu->Window()->EndViewTransaction();
		gCurrentThreadBarMenu->Window()->Flush();
		gCurrentThreadBarMenu->UnlockLooper();
	}

	Window()->EndViewTransaction();
	Window()->Flush();
}
