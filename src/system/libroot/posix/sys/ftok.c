/*
 * Copyright 2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Salvatore Benedetto <salvatore.benedetto@gmail.com>
 */

#include <sys/ipc.h>
#include <sys/stat.h>


key_t
ftok(const char *path, int id)
{
	struct stat st;

	if (stat(path, &st) < 0)
		return (key_t)-1;

	return (key_t)(id << 24 | (st.st_dev & 0xff) << 16 | (st.st_ino & 0xffff));
}
