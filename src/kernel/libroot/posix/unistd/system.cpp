/* 
** Copyright 2004, Ingo Weinhold, bonefish@cs.tu-berlin.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

#include <errno.h>
#include <unistd.h>

#include <OS.h>

#include <syscalls.h>

extern "C"
int
system(const char *command)
{
	if (!command)
		return 1;

	const char *argv[] = { "/bin/sh", "-c", command, NULL };
	int argc = 3;

	team_id team = _kern_create_team(argv[0], argv[0], (char**)argv, argc, NULL,
		0, B_NORMAL_PRIORITY);
	if (team < 0) {
		errno = team;
		return -1;
	}

	status_t returnValue;
	status_t error = _kern_wait_for_team(team, &returnValue);
	if (error != B_OK) {
		errno = error;
		return -1;
	}
	return returnValue;
}
