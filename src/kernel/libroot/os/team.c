/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>
#include "syscalls.h"


status_t
_get_team_usage_info(team_id tmid, int32 who, team_usage_info *ti, size_t size)
{
	// size is not yet used, but may if team_usage_info changes
	(void)size;

	// ToDo: implement _get_team_usage_info
	return B_ERROR;
}


status_t
kill_team(team_id team)  
{
	return sys_kill_team(team);
}


status_t
_get_team_info(team_id team, team_info *info, size_t size)
{
	// size is not yet used, but may if team_info changes
	(void)size;

	return sys_get_team_info(team, info);
}


status_t
_get_next_team_info(int32 *cookie, team_info *info, size_t size)
{
	// size is not yet used, but may if team_info changes
	(void)size;

	return sys_get_next_team_info(cookie, info);
}

