/*
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "TeamInfo.h"


TeamInfo::TeamInfo()
	:
	fTeam(-1),
	fArguments()
{
}


TeamInfo::TeamInfo(const TeamInfo &other)
{
	fTeam = other.fTeam;
	fArguments = other.fArguments;
}


TeamInfo::TeamInfo(team_id team, const team_info& info)
{
	SetTo(team, info);
}


void
TeamInfo::SetTo(team_id team, const team_info& info)
{
	fTeam = team;
	fArguments.SetTo(info.args);
}
