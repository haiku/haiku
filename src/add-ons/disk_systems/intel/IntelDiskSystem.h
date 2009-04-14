/*
 * Copyright 2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef _INTEL_DISK_SYSTEM_H
#define _INTEL_DISK_SYSTEM_H

#include "PartitionMap.h"


static inline off_t
sector_align(off_t offset, uint32 blockSize)
{
	return offset / blockSize * blockSize;
}


#endif	// _INTEL_DISK_SYSTEM_H
