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
			void				_TeamDied(team_id team);

	static	status_t			_BigBrotherEntry(void* self);
			void				_BigBrother();

private:
			typedef std::map<team_id, BMessenger> AppMap;

			AppMap				fMap;
			thread_id			fBigBrother;
			sem_id				fQuit;
};


#endif // APP_MANAGER_H
