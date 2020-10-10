/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ThreadBarMenu.h"

#include "PriorityMenu.h"
#include "ProcessController.h"
#include "ThreadBarMenuItem.h"

#include <stdlib.h>
#include <stdio.h>

#define EXTRA 20


ThreadBarMenu::ThreadBarMenu(const char *title, team_id team, int32 threadCount)
	: BMenu(title),
	fThreadsRecCount(threadCount + EXTRA),
	fTeam(team)
{
	SetFont(be_plain_font);
	fThreadsRec = (ThreadRec*) malloc(sizeof(ThreadRec) * fThreadsRecCount);
	Init();
	fRound = 0;	// for syslog
	AddNew();
}


ThreadBarMenu::~ThreadBarMenu()
{
	free(fThreadsRec);
	if (gCurrentThreadBarMenu == this)
		gCurrentThreadBarMenu = NULL;
}


void
ThreadBarMenu::Init()
{
	int k = 0;
	while (k < fThreadsRecCount)
		fThreadsRec[k++].thread = -1;
	fRound = 1;
}


void
ThreadBarMenu::Reset(team_id team)
{
	fTeam = team;
	RemoveItems(0, CountItems(), true);
	Init();
}


void
ThreadBarMenu::AttachedToWindow()
{
	BMenu::AttachedToWindow();
}


void
ThreadBarMenu::Draw(BRect r)
{
	gCurrentThreadBarMenu = this;
	BMenu::Draw(r);
}


void
ThreadBarMenu::AddNew()
{
	thread_info	info;
	int32 cookie = 0;
	int32 k = 0;
	while (get_next_thread_info(fTeam, &cookie, &info) == B_OK) {
		int	lastk = k;
		while (k < fThreadsRecCount && fThreadsRec[k].thread != info.thread)
			k++;
		if (k == fThreadsRecCount) {
			k = 0;
			while (k < lastk && fThreadsRec[k].thread != info.thread)
				k++;
			if (k == lastk)
				k = fThreadsRecCount; // flag that the search didn't work.
		}
		if (k == fThreadsRecCount) {
//			printf("*** Thread %d %s/%s, user %Ld, kernel %Ld\n", info.thread, info.name, info.user_time, info.kernel_time);
			// this is a new thread...
			k = 0;
			while (k < fThreadsRecCount && !(fThreadsRec[k].thread == -1 || fThreadsRec[k].last_round+1 < fRound))
				k++;
			if (k == fThreadsRecCount) {
				fThreadsRecCount += EXTRA;
				fThreadsRec = (ThreadRec*) realloc(fThreadsRec, sizeof(ThreadRec)*fThreadsRecCount);
				lastk = k;
				while (lastk < fThreadsRecCount)
					fThreadsRec[lastk++].thread = -1;
			}
			fThreadsRec[k].thread = info.thread;
			BMessage* kill_thread = new BMessage('KlTh');
			kill_thread->AddInt32("thread", info.thread);

			PriorityMenu* prio = new PriorityMenu(info.thread, info.priority);
			prio->SetFont(be_plain_font);
			ThreadBarMenuItem* threadbarmenuitem = new ThreadBarMenuItem(info.name, info.thread, prio, kill_thread);
			threadbarmenuitem->SetTarget(gPCView);
			AddItem(threadbarmenuitem);
		}
		fThreadsRec[k].last_round = fRound;
	}
	fRound++;
}


void
ThreadBarMenu::Update()
{
	AddNew();
	int32 k, del;
	del = -1;
	ThreadBarMenuItem *item;

	for (k = 0; (item = (ThreadBarMenuItem*) ItemAt(k)) != NULL; k++) {
		item->BarUpdate();
		item->DrawBar(false);
		if (item->fKernel < 0) {
			if (del < 0)
				del = k;
		} else if (del >= 0) {
			RemoveItems(del, k-del, true);
			k = del;
			del = -1;
		}
	}
	if (del >= 0)
		RemoveItems(del, k-del, true);
}
