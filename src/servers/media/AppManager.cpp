#include <OS.h>
#include <Messenger.h>
#include <Autolock.h>
#include <stdio.h>
#include "AppManager.h"

AppManager::AppManager()
{
}

AppManager::~AppManager()
{
}

bool AppManager::HasTeam(team_id team)
{
	BAutolock lock(mLocker);
	ListItem *item;
	for (int32 i = 0; (item = (ListItem *)mList.ItemAt(i)) != NULL; i++)
		if (item->team == team)
			return true;
	return false;
}

status_t AppManager::RegisterTeam(team_id team, BMessenger messenger)
{
	printf("AppManager::RegisterTeam %d\n",(int) team);

	BAutolock lock(mLocker);
	ListItem *item;

	if (HasTeam(team))
		return B_ERROR;

	item = new ListItem;
	item->team = team;
	item->messenger = messenger;

	return mList.AddItem(item) ? B_OK : B_ERROR;
}

status_t AppManager::UnregisterTeam(team_id team)
{
	printf("AppManager::UnregisterTeam %d\n",(int) team);

	BAutolock lock(mLocker);
	ListItem *item;
	for (int32 i = 0; (item = (ListItem *)mList.ItemAt(i)) != NULL; i++)
		if (item->team == team) {
			if (mList.RemoveItem(item)) {
				delete item;
				return B_OK;
			} else {
				break;
			}
		}
	return B_ERROR;
}

void AppManager::BroadcastMessage(BMessage *msg, bigtime_t timeout)
{
	BAutolock lock(mLocker);
	ListItem *item;
	for (int32 i = 0; (item = (ListItem *)mList.ItemAt(i)) != NULL; i++)
		if (B_OK != item->messenger.SendMessage(msg,(BHandler *)NULL,timeout))
			HandleBroadcastError(msg, item->messenger, item->team, timeout);
}

void AppManager::HandleBroadcastError(BMessage *msg, BMessenger &, team_id team, bigtime_t timeout)
{
	BAutolock lock(mLocker);
	printf("error broadcasting team %d with message after %.3f seconds\n",int(team),timeout / 1000000.0);
	msg->PrintToStream();	
}

status_t AppManager::LoadState()
{
	return B_ERROR;
}

status_t AppManager::SaveState()
{
	return B_ERROR;
}
