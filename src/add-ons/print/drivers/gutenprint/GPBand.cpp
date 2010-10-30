/*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Michael Pfeiffer
*/
#include "GPBand.h"

GPBand::GPBand(BBitmap* bitmap, BRect validRect, BPoint where)
	:
	fBitmap(*bitmap),
	fValidRect(validRect),
	fWhere(where)
{

}


BRect
GPBand::GetBoundingRectangle() const
{
	BRect rect = fValidRect;
	rect.OffsetTo(fWhere);
	return rect;
}


bool
GPBand::ContainsLine(int line) const
{
	int y = line - (int)fWhere.y;
	return 0 <= y && y <= fValidRect.IntegerHeight();
}
