/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdlib.h>

#include <scheduler_defs.h>
#include <syscalls.h>


extern "C" int
getloadavg(double loadavg[], int count)
{
	struct loadavg info;

	if (_kern_get_loadavg(&info, sizeof(info)) != B_OK)
		return -1;

	size_t size = min_c((size_t)count, B_COUNT_OF(info.ldavg));
	for (size_t i = 0; i < size; i++)
		loadavg[i] = (double)info.ldavg[i] / info.fscale;
	return count;
}
