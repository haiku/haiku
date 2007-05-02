/*
 * Copyright 2005-2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_PATH_UTIL_H
#define _FSSH_PATH_UTIL_H

#include "fssh_defs.h"


namespace FSShell {


fssh_status_t	get_last_path_component(const char *path, char *buffer,
					int bufferLen);
char*			make_path(const char *dir, const char *entry);


}	// namespace FSShell


#endif	// _FSSH_PATH_UTIL_H
