/*
 * Copyright, 2016-2017 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT license.
 *
 * Authors:
 * 		Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include <sys/time.h>

#include <OS.h>


extern int _utimes(const char* path, const struct timeval times[2],
	bool traverseLink);


int
lutimes(const char* path, const struct timeval times[2])
{
	return _utimes(path, times, false);
}
