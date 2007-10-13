/*
 * Copyright 2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef _INTEL_DISK_SYSTEM_H
#define _INTEL_DISK_SYSTEM_H

#include "PartitionMap.h"


static inline off_t
sector_align(off_t offset)
{
	return offset / SECTOR_SIZE * SECTOR_SIZE;
}


#endif	// _INTEL_DISK_SYSTEM_H
