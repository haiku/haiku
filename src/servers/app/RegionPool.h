/*
 * Copyright (c) 2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef REGION_POOL_H
#define REGION_POOL_H

#include <List.h>

class BRegion;

#define DEBUG_LEAK 0

class RegionPool {
 public:
								RegionPool();
	virtual						~RegionPool();

			BRegion*			GetRegion();
			BRegion*			GetRegion(const BRegion& other);
			void				Recycle(BRegion* region);

 private:
			BList				fAvailable;
#if DEBUG_LEAK
			BList				fUsed;
#endif
};

#endif // REGION_POOL_H
