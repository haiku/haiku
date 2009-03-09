/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <fs_cache.h>


// TODO: We could propagate these calls to the kernel. They are only used for
// optimization though and are not mission critical.


status_t
entry_cache_add(dev_t mountID, ino_t dirID, const char* name, ino_t nodeID)
{
	return B_OK;
}


status_t
entry_cache_remove(dev_t mountID, ino_t dirID, const char* name)
{
	return B_OK;
}
