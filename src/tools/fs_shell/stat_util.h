/*
 * Copyright 2005-2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_STAT_UTIL_H
#define _FSSH_STAT_UTIL_H

#include <sys/stat.h>

#include "fssh_defs.h"
#include "fssh_stat.h"


namespace FSShell {

mode_t to_platform_mode(fssh_mode_t mode);

void from_platform_stat(const struct stat *st, struct fssh_stat *fsshStat);
void to_platform_stat(const struct fssh_stat *fsshStat, struct stat *st);

extern int to_platform_open_mode(int fsshMode);

}	// namespace FSShell


#endif	// _FSSH_STAT_UTIL_H
