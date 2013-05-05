/*
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEAM_INFO_H
#define TEAM_INFO_H

#include <OS.h>
#include <String.h>

#include "Types.h"


class TeamInfo {
public:
								TeamInfo();
								TeamInfo(const TeamInfo& other);
								TeamInfo(team_id team,
									const team_info& info);

			void				SetTo(team_id team, const team_info& info);

			team_id				TeamID() const	{ return fTeam; }

			const BString&		Arguments() const	{ return fArguments; }

private:
			team_id				fTeam;
			BString				fArguments;
};


#endif // TEAM_INFO_H
