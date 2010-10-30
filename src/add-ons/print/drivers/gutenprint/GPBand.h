/*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Michael Pfeiffer
*/
#ifndef GP_BAND_H
#define GP_BAND_H


#include <Bitmap.h>


class GPBand
{
public:
			GPBand(BBitmap* bitmap, BRect validRect, BPoint where);

	BRect	GetBoundingRectangle() const;
	bool	ContainsLine(int line) const;

	BBitmap	fBitmap;
	BRect	fValidRect;
	BPoint	fWhere;
};

#endif
