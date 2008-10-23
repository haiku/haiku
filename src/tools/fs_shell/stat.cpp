/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */

#include "compatibility.h"

#include "fssh_stat.h"

#include <SupportDefs.h>

#include <sys/stat.h>

#include "fssh_errno.h"
#include "partition_support.h"
#include "stat_priv.h"
#include "stat_util.h"


using FSShell::from_platform_stat;
using FSShell::to_platform_mode;


#if (!defined(__BEOS__) && !defined(__HAIKU__))
	// The _kern_read_stat() defined in libroot_build.so.
	extern "C" status_t _kern_read_stat(int fd, const char *path,
		bool traverseLink, struct stat *st, size_t statSize);
#endif


int
FSShell::unrestricted_stat(const char *path, struct fssh_stat *fsshStat)
{
	struct stat st;

	// Use the _kern_read_stat() defined in libroot on BeOS incompatible
	// systems. Required for support for opening symlinks.
#if (defined(__BEOS__) || defined(__HAIKU__))
	if (::stat(path, &st) < 0)
		return -1;
#else
	status_t error = _kern_read_stat(-1, path, true, &st, sizeof(st));
	if (error < 0) {
		fssh_set_errno(error);
		return -1;
	}
#endif

	from_platform_stat(&st, fsshStat);
	
	return 0;
}


int
FSShell::unrestricted_fstat(int fd, struct fssh_stat *fsshStat)
{
	struct stat st;

	// Use the _kern_read_stat() defined in libroot on BeOS incompatible
	// systems. Required for support for opening symlinks.
#if (defined(__BEOS__) || defined(__HAIKU__))
	if (fstat(fd, &st) < 0)
		return -1;
#else
	status_t error = _kern_read_stat(fd, NULL, false, &st, sizeof(st));
	if (error < 0) {
		fssh_set_errno(error);
		return -1;
	}
#endif

	from_platform_stat(&st, fsshStat);
	
	return 0;
}


int
FSShell::unrestricted_lstat(const char *path, struct fssh_stat *fsshStat)
{
	struct stat st;

	// Use the _kern_read_stat() defined in libroot on BeOS incompatible
	// systems. Required for support for opening symlinks.
#if (defined(__BEOS__) || defined(__HAIKU__))
	if (lstat(path, &st) < 0)
		return -1;
#else
	status_t error = _kern_read_stat(-1, path, false, &st, sizeof(st));
	if (error < 0) {
		fssh_set_errno(error);
		return -1;
	}
#endif

	from_platform_stat(&st, fsshStat);
	
	return 0;
}

// #pragma mark -

int
fssh_mkdir(const char *path, fssh_mode_t mode)
{
	return mkdir(path, to_platform_mode(mode));
}


int
fssh_stat(const char *path, struct fssh_stat *fsshStat)
{
	if (FSShell::unrestricted_stat(path, fsshStat) < 0)
		return -1;

	FSShell::restricted_file_restrict_stat(fsshStat);

	return 0;
}


int
fssh_fstat(int fd, struct fssh_stat *fsshStat)
{
	if (FSShell::unrestricted_fstat(fd, fsshStat) < 0)
		return -1;
	
	FSShell::restricted_file_restrict_stat(fsshStat);

	return 0;
}


int
fssh_lstat(const char *path, struct fssh_stat *fsshStat)
{
	if (FSShell::unrestricted_lstat(path, fsshStat) < 0)
		return -1;
	
	FSShell::restricted_file_restrict_stat(fsshStat);

	return 0;
}
