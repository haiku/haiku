// This file is distributed under the OpenBeOS license

#ifndef __REGION_SUPPORT_H
#define __REGION_SUPPORT_H

#include <Region.h>

class BRegion::Support 
{
public:
	static void ZeroRegion(BRegion *a_region);
	static void ClearRegion(BRegion *a_region);
	static void CopyRegion(BRegion *src_region, BRegion *dst_region);
	static void AndRegion(BRegion *first, BRegion *second, BRegion *dest);
	static void OrRegion(BRegion *first, BRegion *second, BRegion *dest);
	static void SubRegion(BRegion *first, BRegion *second, BRegion *dest);

private:
	static void CleanupRegion(BRegion *region_in);
	static void CleanupRegionVertical(BRegion *region_in);
	static void CleanupRegionHorizontal(BRegion *region_in);
	
	static void SortRects(clipping_rect *rects, long count);
	static void SortTrans(long *lptr1, long *lptr2, long count);	
	
	static void CopyRegionMore(BRegion*, BRegion*, long);
	
	static void AndRegionComplex(BRegion*, BRegion*, BRegion*);
	static void AndRegion1ToN(BRegion*, BRegion*, BRegion*);

	static void AppendRegion(BRegion*, BRegion*, BRegion*);
	
	static void OrRegionComplex(BRegion*, BRegion*, BRegion*);
	static void OrRegion1ToN(BRegion*, BRegion*, BRegion*);
	static void OrRegionNoX(BRegion*, BRegion*, BRegion*);
	static void ROr(long, long, BRegion*, BRegion*, BRegion*, long*, long *);

	static void SubRegionComplex(BRegion*, BRegion*, BRegion*);
	static void RSub(long , long, BRegion*, BRegion*, BRegion*, long*, long*);
};

#endif // __REGION_SUPPORT_H
