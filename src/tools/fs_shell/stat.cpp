/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */

#include <BeOSBuildCompatibility.h>

#include "fssh_stat.h"

#include <sys/stat.h>

#include "stat_util.h"


using FSShell::from_platform_stat;


int
fssh_stat(const char *path, struct fssh_stat *fsshStat)
{
	struct stat st;
	if (stat(path, &st) < 0)
		return -1;

	from_platform_stat(&st, fsshStat);
	
	return 0;
}


int
fssh_fstat(int fd, struct fssh_stat *fsshStat)
{
	struct stat st;
	if (fstat(fd, &st) < 0)
		return -1;

	from_platform_stat(&st, fsshStat);
	
	return 0;
}


int
fssh_lstat(const char *path, struct fssh_stat *fsshStat)
{
	struct stat st;
	if (lstat(path, &st) < 0)
		return -1;

	from_platform_stat(&st, fsshStat);
	
	return 0;
}
