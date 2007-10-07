/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef INTEL_H
#define INTEL_H

#include <SupportDefs.h>

#include "PartitionMap.h"


// A PartitionMap with reference count.
struct PartitionMapCookie : PartitionMap {
	PartitionMapCookie() : ref_count(1) {}

	int32	ref_count;
};


#endif	// INTEL_H
