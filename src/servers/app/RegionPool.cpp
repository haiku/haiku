/*
 * Copyright (c) 2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "RegionPool.h"

#include <new>
#include <stdio.h>

#if DEBUG_LEAK
#include <debugger.h>
#endif

#include <Region.h>

using std::nothrow;

RegionPool::RegionPool()
	: fAvailable(4)
#if DEBUG_LEAK
	  ,fUsed(4)
#endif
{
}


RegionPool::~RegionPool()
{
#if DEBUG_LEAK
	if (fUsed.CountItems() > 0)
		debugger("RegionPool::~RegionPool() - some regions still in use!");
#endif
	int32 count = fAvailable.CountItems();
	for (int32 i = 0; i < count; i++)
		delete (BRegion*)fAvailable.ItemAtFast(i);
}


BRegion*
RegionPool::GetRegion()
{
	BRegion* region = (BRegion*)fAvailable.RemoveItem(
		fAvailable.CountItems() - 1);
	if (!region) {
		region = new (nothrow) BRegion();
		if (!region) {
			// whoa
			fprintf(stderr, "RegionPool::GetRegion() - "
							"no memory!\n");
		}
	}
#if DEBUG_LEAK
	fUsed.AddItem(region);
#endif
	return region;
}


BRegion*
RegionPool::GetRegion(const BRegion& other)
{
	BRegion* region;
	int32 count = fAvailable.CountItems();
	if (count > 0) {
		region = (BRegion*)fAvailable.RemoveItem(count - 1);
		*region = other;
	} else {
		region = new (nothrow) BRegion(other);
		if (!region) {
			// whoa
			fprintf(stderr, "RegionPool::GetRegion() - "
							"no memory!\n");
		}
	}

#if DEBUG_LEAK
	fUsed.AddItem(region);
#endif
	return region;
}


void
RegionPool::Recycle(BRegion* region)
{
	if (!fAvailable.AddItem(region)) {
		// at least don't leak the region...
		fprintf(stderr, "RegionPool::Recycle() - "
						"no memory!\n");
		delete region;
	} else {
		// prepare for next usage
		region->MakeEmpty();
	}
#if DEBUG_LEAK
	fUsed.RemoveItem(region);
#endif
}

