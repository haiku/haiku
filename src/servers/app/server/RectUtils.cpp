#include "RectUtils.h"

bool TestRectIntersection(const BRect &r,const BRect &r2)
{
	return	TestLineIntersection(r, r2.left, r2.top, r2.left, r2.bottom)		||
			TestLineIntersection(r, r2.left, r2.top, r2.right, r2.top, false)	||
			TestLineIntersection(r, r2.right, r2.top, r2.right, r2.bottom)		||
			TestLineIntersection(r, r2.left, r2.bottom, r2.right, r2.bottom, false) ||
			r.Contains(r2) ||
			r2.Contains(r);
}

bool TestRegionIntersection(BRegion *r,const BRect &r2)
{
	for(int32 i=0; i<r->CountRects(); i++)
		if(TestRectIntersection(r->RectAt(i),r2));
			return true;
	return false;
}

void IntersectRegionWith(BRegion *r,const BRect &r2)
{
	// We have three conditions:
	// 1) Region frame contains rect. Action: call Include()
	// 2) Rect intersects region frame. Action: call IntersectWith
	// 3) Region frame does not intersect rectangle. Make the region empty
	if(r->Frame().Contains(r2))
		r->Include(r2);
	if(r->Frame().Intersects(r2))
	{
		BRegion reg(r2);
		r->IntersectWith(&reg);
	}
	else
		r->MakeEmpty();
}

bool TestLineIntersection(const BRect& r, float x1, float y1, float x2, float y2,
					   bool vertical)
{
	if (vertical)
	{
		return	(x1 >= r.left && x1 <= r.right) &&
				((y1 >= r.top && y1 <= r.bottom) ||
				 (y2 >= r.top && y2 <= r.bottom));
	}
	else
	{
		return	(y1 >= r.top && y1 <= r.bottom) &&
				((x1 >= r.left && x1 <= r.right) ||
				 (x2 >= r.left && x2 <= r.right));
	}
}

void ValidateRect(BRect *rect)
{
	float l,r,t,b;
	l=(rect->left<rect->right)?rect->left:rect->right;
	r=(rect->left>rect->right)?rect->left:rect->right;
	t=(rect->top<rect->bottom)?rect->top:rect->bottom;
	b=(rect->top>rect->bottom)?rect->top:rect->bottom;
	rect->Set(l,t,r,b);
}
