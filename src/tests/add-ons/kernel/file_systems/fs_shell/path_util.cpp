/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include "compat.h"

#include <stdlib.h>
#include <string.h>

#include "path_util.h"

status_t
get_last_path_component(const char *path, char *buffer, int bufferLen)
{
	int len = strlen(path);
	if (len == 0)
		return FS_EINVAL;

	// eat trailing '/'
	while (len > 0 && path[len - 1] == '/')
		len--;

	if (len == 0) {
		// path is `/'
		len = 1;
	} else {
		// find previous '/'
		int pos = len - 1;
		while (pos > 0 && path[pos] != '/')
			pos--;
		if (path[pos] == '/')
			pos++;

		path += pos;
		len -= pos;
	}

	if (len >= bufferLen)
		return FS_NAME_TOO_LONG;

	memcpy(buffer, path, len);
	buffer[len] = '\0';
	return FS_OK;
}

char *
make_path(const char *dir, const char *entry)
{
	// get the len
	int dirLen = strlen(dir);
	int entryLen = strlen(entry);
	bool insertSeparator = (dir[dirLen - 1] != '/');
	int pathLen = dirLen + entryLen + (insertSeparator ? 1 : 0) + 1;

	// allocate the path
	char *path = (char*)malloc(pathLen);
	if (!path)
		return NULL;

	// compose the path
	strcpy(path, dir);
	if (insertSeparator)
		strcat(path + dirLen, "/");
	strcat(path + dirLen, entry);

	return path;
}


