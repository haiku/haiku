/*
 * Copyright (c) 2002, 2003 Marcus Overhagen <Marcus@Overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */


#include "AppManager.h"

#include <stdio.h>

#include <Application.h>
#include <Autolock.h>
#include <OS.h>
#include <Roster.h>

#include <debug.h>
#include <MediaMisc.h>

#include "BufferManager.h"
#include "media_server.h"
#include "NodeManager.h"
#include "NotificationManager.h"


AppManager::AppManager()
	:
	BLocker("media app manager")
{
	fQuit = create_sem(0, "big brother waits");
	fBigBrother = spawn_thread(_BigBrotherEntry, "big brother is watching you",
		B_NORMAL_PRIORITY, this);
	resume_thread(fBigBrother);
}


AppManager::~AppManager()
{
	delete_sem(fQuit);
	wait_for_thread(fBigBrother, NULL);
}


bool
AppManager::HasTeam(team_id team)
{
	BAutolock lock(this);
	return fMap.find(team) != fMap.end();
}


status_t
AppManager::RegisterTeam(team_id team, const BMessenger& messenger)
{
	BAutolock lock(this);

	TRACE("AppManager::RegisterTeam %ld\n", team);
	if (HasTeam(team)) {
		ERROR("AppManager::RegisterTeam: team %ld already registered\n", team);
		return B_ERROR;
	}

	try {
		fMap.insert(std::make_pair(team, messenger));
	} catch (std::bad_alloc& exception) {
		return B_NO_MEMORY;
	}

	return B_OK;
}


status_t
AppManager::UnregisterTeam(team_id team)
{
	TRACE("AppManager::UnregisterTeam %ld\n", team);

	Lock();
	bool isRemoved = fMap.erase(team) != 0;
	Unlock();

	_CleanupTeam(team);

	return isRemoved ? B_OK : B_ERROR;
}


team_id
AppManager::AddOnServerTeam()
{
	team_id id = be_roster->TeamFor(B_MEDIA_ADDON_SERVER_SIGNATURE);
	if (id < 0) {
		ERROR("media_server: Trouble, media_addon_server is dead!\n");
		return -1;
	}
	return id;
}


status_t
AppManager::SendMessage(team_id team, BMessage* message)
{
	BAutolock lock(this);

	AppMap::iterator found = fMap.find(team);
	if (found == fMap.end())
		return B_NAME_NOT_FOUND;

	return found->second.SendMessage(message);
}


void
AppManager::Dump()
{
	BAutolock lock(this);

	printf("\n");
	printf("AppManager: list of applications follows:\n");

	app_info info;
	AppMap::iterator iterator = fMap.begin();
	for (; iterator != fMap.end(); iterator++) {
		app_info info;
		be_roster->GetRunningAppInfo(iterator->first, &info);
		printf(" team %ld \"%s\", messenger %svalid\n", iterator->first,
			info.ref.name, iterator->second.IsValid() ? "" : "NOT ");
	}

	printf("AppManager: list end\n");
}


void
AppManager::_CleanupTeam(team_id team)
{
	ASSERT(!IsLocked());

	TRACE("AppManager: cleaning up team %ld\n", team);

	gNodeManager->CleanupTeam(team);
	gBufferManager->CleanupTeam(team);
	gNotificationManager->CleanupTeam(team);
}


void
AppManager::_TeamDied(team_id team)
{
	UnregisterTeam(team);
}


status_t
AppManager::_BigBrotherEntry(void* self)
{
	static_cast<AppManager*>(self)->_BigBrother();
	return 0;
}


/*!	The BigBrother thread send ping messages to the BMediaRoster of
	all currently running teams. If the reply times out or is wrong,
	the team cleanup function _TeamDied() will be called.
*/
void
AppManager::_BigBrother()
{
	status_t status = B_TIMED_OUT;
	BMessage ping('PING');
	BMessage reply;

	do {
		if (!Lock())
			break;

		bool startOver = false;
		AppMap::iterator iterator = fMap.begin();
		for (; iterator != fMap.end(); iterator++) {
			reply.what = 0;
			status = iterator->second.SendMessage(&ping, &reply, 5000000,
				2000000);
			if (status != B_OK || reply.what != 'PONG') {
				team_id team = iterator->first;
				Unlock();

				_TeamDied(team);
				startOver = true;
				break;
			}
		}

		if (startOver)
			continue;

		Unlock();
		status = acquire_sem_etc(fQuit, 1, B_RELATIVE_TIMEOUT, 2000000);
	} while (status == B_TIMED_OUT || status == B_INTERRUPTED);
}
