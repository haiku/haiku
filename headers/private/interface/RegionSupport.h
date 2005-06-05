// This file is distributed under the OpenBeOS license

#ifndef __REGION_SUPPORT_H
#define __REGION_SUPPORT_H

#include <Region.h>

class BRegion::Support {
public:
	static void ZeroRegion(BRegion &region);
	static void ClearRegion(BRegion &region);
	static void CopyRegion(const BRegion &source, BRegion &dest);
	static void AndRegion(const BRegion &first, const BRegion &second, BRegion &dest);
	static void OrRegion(const BRegion &first, const BRegion &second, BRegion &dest);
	static void SubRegion(const BRegion &first, const BRegion &second, BRegion &dest);

private:
	static void CleanupRegion(BRegion &region);
	static void CleanupRegionVertical(BRegion &region);
	static void CleanupRegionHorizontal(BRegion &region);
	
	static void SortRects(clipping_rect *rects, long count);
	static void SortTrans(long *lptr1, long *lptr2, long count);	
	
	static void CopyRegionMore(const BRegion &, BRegion &, long);
	
	static void AndRegionComplex(const BRegion &, const BRegion &, BRegion &);
	static void AndRegion1ToN(const BRegion &, const BRegion &, BRegion &);

	static void AppendRegion(const BRegion &, const BRegion &, BRegion &);
	
	static void OrRegionComplex(const BRegion &, const BRegion &, BRegion &);
	static void OrRegion1ToN(const BRegion &, const BRegion &, BRegion &);
	static void OrRegionNoX(const BRegion &, const BRegion &, BRegion &);
	static void ROr(long, long, const BRegion &, const BRegion &,
					BRegion &, long *, long *);

	static void SubRegionComplex(const BRegion &, const BRegion &, BRegion &);
	static void RSub(long, long, const BRegion &, const BRegion &,
					BRegion &, long *, long *);
};

#endif // __REGION_SUPPORT_H
