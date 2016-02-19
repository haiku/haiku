/*
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef APP_MANAGER_H
#define APP_MANAGER_H


#include <map>

#include <Locker.h>
#include <Messenger.h>


class AppManager : BLocker {
public:
								AppManager();
								~AppManager();

			status_t			RegisterTeam(team_id team,
									const BMessenger& messenger);
			status_t			UnregisterTeam(team_id team);
			bool				HasTeam(team_id team);

			team_id				AddOnServerTeam();

			status_t			SendMessage(team_id team, BMessage* message);

			void				Dump();

private:
			void				_CleanupTeam(team_id team);

private:
			typedef std::map<team_id, BMessenger> AppMap;

			AppMap				fMap;
};


#endif // APP_MANAGER_H
