/*
** Copyright 2004, Jérôme Duval, jerome.duval@free.fr.
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <OS.h>
#include <sys/resource.h>
#include <errno.h>

#include <errno_private.h>
#include <symbol_versioning.h>


// prototypes for the compiler
int _getrusage_base(int who, struct rusage *rusage);
int _getrusage_current(int who, struct rusage *rusage);


int
_getrusage_base(int who, struct rusage *rusage)
{
	team_usage_info info;

	if (get_team_usage_info(B_CURRENT_TEAM, who, &info) != B_OK) {
		__set_errno(B_BAD_VALUE);
		return -1;
	}

	rusage->ru_utime.tv_sec = info.user_time / 1000000;
	rusage->ru_utime.tv_usec = info.user_time % 1000000;

	rusage->ru_stime.tv_sec = info.kernel_time / 1000000;
	rusage->ru_stime.tv_usec = info.kernel_time % 1000000;

	return 0;
}


int
_getrusage_current(int who, struct rusage *rusage)
{
	int err = _getrusage_base(who, rusage);
	if (err != -1) {
		memset(&rusage->ru_maxrss, 0, sizeof(struct rusage) -
			offsetof(struct rusage, ru_maxrss));
	}
	return err;
}


DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("_getrusage_base", "getrusage@", "BASE");
DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("_getrusage_current", "getrusage@@",
	"1_BETA3");
