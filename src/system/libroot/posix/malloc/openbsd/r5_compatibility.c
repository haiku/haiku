/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <SupportDefs.h>


#ifdef __HAIKU_BEOS_COMPATIBLE


struct mstats {
	size_t bytes_total;
	size_t chunks_used;
	size_t bytes_used;
	size_t chunks_free;
	size_t bytes_free;
};


struct mstats
mstats()
{
	struct mstats stats = {};
	return stats;
}


#endif
