#ifndef RECTUTILS_H_
#define RECTUTILS_H_

#include <Rect.h>
#include <Region.h>

bool TestLineIntersection(const BRect&r, float x1, float y1, float x2, float y2,bool vertical=true);
bool TestRectIntersection(const BRect &r,const BRect &r2);
bool TestRegionIntersection(BRegion *r,const BRect &r2);
void IntersectRegionWith(BRegion *r,const BRect &r2);
void ValidateRect(BRect *r);
#endif