/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#ifndef FS_SHELL_PATH_UTIL_H
#define FS_SHELL_PATH_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

status_t get_last_path_component(const char *path, char *buffer, int bufferLen);
char *make_path(const char *dir, const char *entry);

#ifdef __cplusplus
}
#endif

#endif	// FS_SHELL_PATH_UTIL_H
