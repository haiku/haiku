/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_STAT_PRIV_H
#define _FSSH_STAT_PRIV_H

#include "fssh_defs.h"


namespace FSShell {

int unrestricted_stat(const char *path, struct fssh_stat *fsshStat);
int unrestricted_fstat(int fd, struct fssh_stat *fsshStat);
int unrestricted_lstat(const char *path, struct fssh_stat *fsshStat);

}	// namespace FSShell


#endif	// _FSSH_STAT_PRIV_H
