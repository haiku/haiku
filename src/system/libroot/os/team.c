/* 
** Copyright 2002-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include <OS.h>
#include "syscalls.h"


// this call does not exist in BeOS R5
#if 0
status_t
wait_for_team(team_id team, status_t *_returnCode)
{
	return _kern_wait_for_team(team, _returnCode);
}
#endif


status_t
_get_team_usage_info(team_id team, int32 who, team_usage_info *info, size_t size)
{
	return _kern_get_team_usage_info(team, who, info, size);
}


status_t
kill_team(team_id team)
{
	return _kern_kill_team(team);
}


status_t
_get_team_info(team_id team, team_info *info, size_t size)
{
	if (info == NULL || size != sizeof(team_info))
		return B_BAD_VALUE;

	return _kern_get_team_info(team, info);
}


status_t
_get_next_team_info(int32 *cookie, team_info *info, size_t size)
{
	if (info == NULL || size != sizeof(team_info))
		return B_BAD_VALUE;

	return _kern_get_next_team_info(cookie, info);
}

