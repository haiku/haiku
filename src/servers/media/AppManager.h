/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __APP_MANAGER_H
#define __APP_MANAGER_H

#include "TMap.h"

class AppManager
{
public:
				AppManager();
				~AppManager();

	status_t	RegisterTeam(team_id, BMessenger);
	status_t	UnregisterTeam(team_id);
	bool		HasTeam(team_id);

	team_id		AddonServerTeam();
		
	status_t	SendMessage(team_id team, BMessage *msg);
	
	void		Dump();	
	
private:
	void		CleanupTeam(team_id);
	void		TeamDied(team_id team);
	static int32 bigbrother(void *self);
	void		BigBrother();

private:
	thread_id fBigBrother;
	sem_id fQuit;

	struct App {
		team_id team;
		BMessenger messenger;
	};
	Map<team_id, App> * fAppMap;
	BLocker	*fLocker;
};

#endif // __APP_MANAGER_H
