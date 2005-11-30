#ifndef __CLIPREGION_H
#define __CLIPREGION_H

#include "Region.h" // for clipping_rect

class ServerLink;

class ClipRegion {
public:
	ClipRegion();
	ClipRegion(const ClipRegion &region);
	ClipRegion(const BRegion &region);
	ClipRegion(const clipping_rect &rect);
	ClipRegion(const BRect &rect);
	ClipRegion(const clipping_rect *rects, const int32 &count);
	
	~ClipRegion();
	
	clipping_rect Frame() const { return fBound; }
	clipping_rect RectAt(const int32 &index) const;
	
	int32 CountRects() const { return fCount; }
	
	void Set(const clipping_rect &rect);
	void Set(const BRect &rect);
	void Set(const clipping_rect *rects, const int32 &count);
	
	bool Intersects(const clipping_rect &rect) const;
	bool Intersects(const BRect &rect) const;
	bool Intersects(const ClipRegion &region) const;
	
	bool Contains(const BPoint &point) const;
	
	void OffsetBy(const int32 &dx, const int32 &dy);
	void OffsetBy(const BPoint &point);
	
	void Include(const clipping_rect &rect);
	void Include(const BRect &rect);
	void Include(const ClipRegion &region);
	
	void Exclude(const clipping_rect &rect);
	void Exclude(const BRect &rect);
	void Exclude(const ClipRegion &region);
	
	void IntersectWith(const clipping_rect &rect);
	void IntersectWith(const BRect &rect);
	void IntersectWith(const ClipRegion &region);
	
	ClipRegion &operator=(const ClipRegion &region);
	
	status_t ReadFromLink(ServerLink &link);
	status_t WriteToLink(ServerLink &link);
	
private:
	int32	fCount;
	int32	fDataSize;
	clipping_rect	fBound;
	clipping_rect	*fData;
	
	void _Append(const ClipRegion &region, const bool &aboveThis = false);
	void _Append(const clipping_rect *rects, const int32 &count, const bool &aboveThis = false);
	
	void _IntersectWithComplex(const ClipRegion &region);
	void _IntersectWith(const clipping_rect &rect);
	void _IncludeComplex(const ClipRegion &region);
	void _ExcludeComplex(const ClipRegion &region);
	
	void _SortRects();
	void _AddRect(const clipping_rect &rect);
	void _RecalculateBounds(const clipping_rect &newRect);
	
	int32 _FindSmallestBottom(const int32 &top, const int32 &startIndex) const;

	void _Invalidate();
	bool _Resize(const int32 &newSize, const bool &keepOld = true);
	void _Adopt(ClipRegion &region);
};


inline clipping_rect
ClipRegion::RectAt(const int32 &index) const 
{
	if (index >= 0 && index < fCount)
		return fData[index];
	clipping_rect rect = { 0, 0, -1, -1 };
	return rect;
}

#endif // __CLIPREGION_H
