/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "TMap.h"

class AppManager
{
public:
	AppManager();
	~AppManager();

	status_t RegisterAddonServer(team_id);
	status_t RegisterTeam(team_id, BMessenger);
	status_t UnregisterTeam(team_id);
	bool HasTeam(team_id);
	void StartAddonServer();
	void TerminateAddonServer();
	
	void Dump();	
	
private:
	void CleanupTeam(team_id);
	void CleanupAddonServer();
	void TeamDied(team_id team);
	void RestartAddonServer();
	static int32 bigbrother(void *self);
	void BigBrother();

private:
	team_id fAddonServer;
	thread_id fBigBrother;
	sem_id fQuit;

	struct App {
		team_id team;
		BMessenger messenger;
	};
	Map<team_id, App> * fAppMap;
	BLocker	*fLocker;
};
