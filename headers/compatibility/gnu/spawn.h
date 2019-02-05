/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the Haiku License.
 */
#ifndef _GNU_SPAWN_H_
#define _GNU_SPAWN_H_


#include_next <spawn.h>


#ifdef _GNU_SOURCE


#ifdef __cplusplus
extern "C" {
#endif

extern int posix_spawn_file_actions_addchdir_np(
	posix_spawn_file_actions_t *file_actions, const char *path);
extern int posix_spawn_file_actions_addfchdir_np(
	posix_spawn_file_actions_t *file_actions, int fildes);

#ifdef __cplusplus
}
#endif


#endif


#endif	/* _GNU_SPAWN_H_ */

