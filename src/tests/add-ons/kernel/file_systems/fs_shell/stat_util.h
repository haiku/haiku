/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef FS_SHELL_STAT_UTIL_H
#define FS_SHELL_STAT_UTIL_H

#include "compat.h"

#ifdef __cplusplus
extern "C" {
#endif

my_mode_t from_platform_mode(mode_t mode);
mode_t to_platform_mode(my_mode_t mode);

void from_platform_stat(const struct stat *st, struct my_stat *myst);
void to_platform_stat(const struct my_stat *myst, struct stat *st);

extern int to_platform_open_mode(int myMode);

#ifdef __cplusplus
}
#endif


#endif	// FS_SHELL_STAT_UTIL_H
