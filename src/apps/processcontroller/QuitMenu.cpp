/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "QuitMenu.h"
#include "IconMenuItem.h"
#include "ProcessController.h"

#include <Roster.h>
#include <Window.h>
#include <stdio.h>


class QuitMenuItem : public IconMenuItem {
	public:
				QuitMenuItem(team_id team, BBitmap* icon, const char* title,
					BMessage* m, bool purge = false);
		team_id	Team() { return fTeam; }

	private:
		team_id	fTeam;
};


QuitMenuItem::QuitMenuItem(team_id team, BBitmap* icon, const char* title,
	BMessage* m, bool purge)
	:
	IconMenuItem(icon, title, m, true, purge), fTeam(team)
{
}


//	#pragma mark -


QuitMenu::QuitMenu(const char* title, info_pack* infos, int infosCount)
	: BMenu(title),
	fInfos(infos),
	fInfosCount(infosCount),
	fMe(NULL)
{
	SetTargetForItems(gPCView);
}


void
QuitMenu::AttachedToWindow()
{
	if (!fMe)
		fMe = new BMessenger(this);

	be_roster->StartWatching(*fMe, B_REQUEST_LAUNCHED | B_REQUEST_QUIT);
	BList apps;
	team_id tmid;
	be_roster->GetAppList(&apps);
	for (int t = CountItems() - 1; t >= 0; t--) {
		QuitMenuItem* item = (QuitMenuItem*)ItemAt(t);
		bool found = false;
		for (int a = 0; !found && (tmid = (team_id)(addr_t)apps.ItemAt(a)) != 0; a++)
			if (item->Team() == tmid)
				found = true;
		if (!found)
			RemoveItem(t);
	}
	for (int a = 0; (tmid = (team_id)(addr_t) apps.ItemAt(a)) != 0; a++) {
		AddTeam(tmid);
	}

	BMenu::AttachedToWindow();
}


void
QuitMenu::DetachedFromWindow()
{
	BMenu::DetachedFromWindow();
	be_roster->StopWatching(*fMe);
	delete fMe;
	fMe = NULL;
}


void
QuitMenu::AddTeam(team_id tmid)
{
	int	t = 0;
	QuitMenuItem* item;
	while ((item = (QuitMenuItem*) ItemAt(t++)) != NULL) {
		if (item->Team() == tmid)
			return;
	}

	t = 0;
	while (t < fInfosCount && tmid != fInfos[t].team_info.team) {
		t++;
	}

	BMessage* message = new BMessage ('QtTm');
	message->AddInt32 ("team", tmid);
	item = NULL;
	if (t < fInfosCount)
		item = new QuitMenuItem(tmid, fInfos[t].team_icon, fInfos[t].team_name,
			message);
	else {
		info_pack infos;
		if (get_team_info(tmid, &infos.team_info) == B_OK
			&& get_team_name_and_icon(infos, true))
			item = new QuitMenuItem(tmid, infos.team_icon, infos.team_name,
				message, true);
	}
	if (item) {
		item->SetTarget(gPCView);
		AddItem(item);
	} else
		delete message;
}


void
QuitMenu::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case B_SOME_APP_LAUNCHED:
		{
			int32 tmid;
			if (msg->FindInt32("be:team", &tmid) == B_OK)
				AddTeam(tmid);
			break;
		}
		case B_SOME_APP_QUIT:
		{
			int32 tmid;
			if (msg->FindInt32("be:team", &tmid) == B_OK) {
				QuitMenuItem* item;
				int t = 0;
				while ((item = (QuitMenuItem*) ItemAt(t++)) != NULL) {
					if (item->Team() == tmid) {
						delete RemoveItem(--t);
						return;
					}
				}
			}
			break;
		}

		default:
			BMenu::MessageReceived(msg);
	}
}
