/*
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "SystemInfo.h"


SystemInfo::SystemInfo()
	:
	fTeam(-1)
{
	memset(&fSystemInfo, 0, sizeof(system_info));
	memset(&fSystemName, 0, sizeof(utsname));
}


SystemInfo::SystemInfo(const SystemInfo &other)
{
	SetTo(other.fTeam, other.fSystemInfo, other.fSystemName);
}


SystemInfo::SystemInfo(team_id team, const system_info& info,
	const utsname& name)
{
	SetTo(team, info, name);
}


void
SystemInfo::SetTo(team_id team, const system_info& info, const utsname& name)
{
	fTeam = team;
	memcpy(&fSystemInfo, &info, sizeof(system_info));
	memcpy(&fSystemName, &name, sizeof(utsname));
}
