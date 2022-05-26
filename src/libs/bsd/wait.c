/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Bruno Albuquerque, bga@bug-br.org.br
 */

#include <OS.h>

#include <sys/resource.h>
#include <sys/wait.h>


extern pid_t _waitpid(pid_t pid, int* _status, int options,
		team_usage_info *usage_info);

// prototypes for the compiler
pid_t _wait3_base(int *status, int options, struct rusage *rusage);
pid_t _wait4_base(pid_t pid, int *status, int options, struct rusage *rusage);
pid_t _wait3_current(int *status, int options, struct rusage *rusage);
pid_t _wait4_current(pid_t pid, int *status, int options,
	struct rusage *rusage);


pid_t
_wait3_base(int *status, int options, struct rusage *rusage)
{
	return _wait4_base(-1, status, options, rusage);
}


pid_t
_wait4_base(pid_t pid, int *status, int options, struct rusage *rusage)
{
	team_usage_info info;
	pid_t waitPid = _waitpid(pid, status, options,
		rusage != NULL ? &info : NULL);
	if (waitPid != -1 && rusage != NULL) {
		rusage->ru_utime.tv_sec = info.user_time / 1000000;
		rusage->ru_utime.tv_usec = info.user_time % 1000000;

		rusage->ru_stime.tv_sec = info.kernel_time / 1000000;
		rusage->ru_stime.tv_usec = info.kernel_time % 1000000;
	}

	return waitPid;
}


pid_t
_wait3_current(int *status, int options, struct rusage *rusage)
{
	return _wait4_current(-1, status, options, rusage);
}


pid_t
_wait4_current(pid_t pid, int *status, int options, struct rusage *rusage)
{
	pid_t waitPid = _wait4_base(pid, status, options, rusage);
	if (waitPid != -1 && rusage != NULL) {
		memset(&rusage->ru_maxrss, 0, sizeof(struct rusage) -
			offsetof(struct rusage, ru_maxrss));
	}

	return waitPid;
}


#define DEFINE_LIBBSD_SYMBOL_VERSION(function, symbol, version) \
	B_DEFINE_SYMBOL_VERSION(function, symbol "LIBBSD_" version)

DEFINE_LIBBSD_SYMBOL_VERSION("_wait3_base", "wait3@", "BASE");
DEFINE_LIBBSD_SYMBOL_VERSION("_wait4_base", "wait4@", "BASE");
DEFINE_LIBBSD_SYMBOL_VERSION("_wait3_current", "wait3@@", "1_BETA3");
DEFINE_LIBBSD_SYMBOL_VERSION("_wait4_current", "wait4@@", "1_BETA3");

