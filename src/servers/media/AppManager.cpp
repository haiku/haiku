/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
 
#define DEBUG 1
 
#include <OS.h>
#include <Application.h>
#include <Roster.h>
#include <Directory.h>
#include <Entry.h>
#include <Messenger.h>
#include <Autolock.h>
#include <stdio.h>
#include <Debug.h>
#include "AppManager.h"

AppManager::AppManager()
 :	fAddonServer(-1)
{
	fAppMap = new Map<team_id, App>;
	fLocker = new BLocker;
	fQuit = create_sem(0, "big brother waits");
	fBigBrother = spawn_thread(bigbrother, "big brother is watching you", B_NORMAL_PRIORITY, this);
	resume_thread(fBigBrother);
}

AppManager::~AppManager()
{
	status_t err;
	delete_sem(fQuit);
	wait_for_thread(fBigBrother, &err);
	delete fLocker;
	delete fAppMap;
}

bool AppManager::HasTeam(team_id team)
{
	BAutolock lock(fLocker);
	App app;
	return fAppMap->Get(team, &app);
}

status_t AppManager::RegisterTeam(team_id team, BMessenger messenger)
{
	BAutolock lock(fLocker);
	printf("AppManager::RegisterTeam %ld\n", team);
	if (HasTeam(team))
		return B_ERROR;
	App app;
	app.team = team;
	app.messenger = messenger;
	return fAppMap->Insert(team, app) ? B_OK : B_ERROR;
}

status_t AppManager::UnregisterTeam(team_id team)
{
	bool is_removed;
	bool is_addon_server;
	
	printf("AppManager::UnregisterTeam %ld\n", team);
	
	fLocker->Lock();
	is_removed = fAppMap->Remove(team);
	is_addon_server = fAddonServer == team;
	if (is_addon_server)
		fAddonServer = -1;
	fLocker->Unlock();
	
	CleanupTeam(team);
	if (is_addon_server)
		CleanupAddonServer();
	
	return is_removed ? B_OK : B_ERROR;
}

void AppManager::RestartAddonServer()
{
	static bigtime_t restart_period = 0;
	static int restart_tries = 0;
	restart_tries++;
	
	if (((system_time() - restart_period) > 60000000LL) && (restart_tries < 5)) {
		restart_period = system_time();
		restart_tries = 0;
	}
	if (restart_tries < 5) {
		printf("AppManager: Restarting media_addon_server...\n");
		// XXX fixme. We should wait until it is *really* gone
		snooze(5000000);
		StartAddonServer();
	} else {
		printf("AppManager: media_addon_server crashed too often, not restarted\n");
	}
}


void AppManager::TeamDied(team_id team)
{
	CleanupTeam(team);
	fLocker->Lock();
	fAppMap->Remove(team);
	fLocker->Unlock();
}

status_t AppManager::RegisterAddonServer(team_id team)
{
	BAutolock lock(fLocker);
	if (fAddonServer != -1)
		return B_ERROR;
	fAddonServer = team;
	return B_OK;
}

//=========================================================================
// The BigBrother thread send ping messages to the BMediaRoster of
// all currently running teams. If the reply times out or is wrong,
// the team cleanup function TeamDied() will be called. If the dead
// team is the media_addon_server, additionally CleanupAddonServer()
// will be called and also RestartAddonServer()
//=========================================================================

int32 AppManager::bigbrother(void *self)
{
	static_cast<AppManager *>(self)->BigBrother();
	return 0;
}

void AppManager::BigBrother()
{
	bool restart_addon_server;
	status_t status;
	BMessage msg('PING');
	BMessage reply;
	team_id team;
	App *app;
	do {
		if (!fLocker->Lock())
			break;
		for (int32 i = 0; fAppMap->GetPointerAt(i, &app); i++) {
			reply.what = 0;
			status = app->messenger.SendMessage(&msg, &reply, 5000000, 2000000);
			if (status != B_OK || reply.what != 'PONG') {
				team = app->team;
				if (fAddonServer == team) {
					restart_addon_server = true;
					fAddonServer = -1;
				} else {
					restart_addon_server = false;
				}
				fLocker->Unlock();
				TeamDied(team);
				if (restart_addon_server) {
					CleanupAddonServer();
					RestartAddonServer();
				}
				continue;
			}
		}
		fLocker->Unlock();
		status = acquire_sem_etc(fQuit, 1, B_RELATIVE_TIMEOUT, 2000000);
	} while (status == B_TIMED_OUT || status == B_INTERRUPTED);
}

//=========================================================================
// The following functions must be called unlocked.
// They clean up after a crash, or start/terminate the media_addon_server.
//=========================================================================

void AppManager::CleanupTeam(team_id team)
{
	ASSERT(false == fLocker->IsLocked());

	printf("AppManager: cleaning up team %ld\n", team);

}

void AppManager::CleanupAddonServer()
{
	ASSERT(false == fLocker->IsLocked());

	printf("AppManager: cleaning up media_addon_server\n");

}

void AppManager::StartAddonServer()
{
	ASSERT(false == fLocker->IsLocked());

	app_info info;
	be_app->GetAppInfo(&info);
	BEntry entry(&info.ref);
	entry.GetParent(&entry);
	BDirectory dir(&entry);
	entry.SetTo(&dir, "media_addon_server");
	entry_ref ref;
	entry.GetRef(&ref);
	be_roster->Launch(&ref);
}

void AppManager::TerminateAddonServer()
{
	ASSERT(false == fLocker->IsLocked());

	if (fAddonServer != -1) {
		BMessenger msger(NULL, fAddonServer);
		msger.SendMessage(B_QUIT_REQUESTED);
		// XXX fixme. We should wait until it is gone
		snooze(1000000);
	}
}
