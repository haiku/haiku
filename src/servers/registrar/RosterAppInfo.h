/*
 * Copyright 2001-2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef ROSTER_APP_INFO_H
#define ROSTER_APP_INFO_H

#include <Roster.h>

enum application_state {
	APP_STATE_UNREGISTERED,
	APP_STATE_PRE_REGISTERED,
	APP_STATE_REGISTERED,
};


struct RosterAppInfo : app_info {
	application_state	state;
	uint32				token;
		// token is meaningful only if state is APP_STATE_PRE_REGISTERED and
		// team is -1.
	bigtime_t			registration_time;	// time of first addition

	RosterAppInfo();
	void Init(thread_id thread, team_id team, port_id port, uint32 flags,
		const entry_ref *ref, const char *signature);

	RosterAppInfo *Clone() const;
	bool IsRunning() const;
};

#endif	// ROSTER_APP_INFO_H
