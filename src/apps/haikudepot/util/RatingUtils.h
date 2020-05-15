/*
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef RATING_UTILS_H
#define RATING_UTILS_H


#include <Referenceable.h>


#include "SharedBitmap.h"


class BView;
class BBitmap;


class RatingUtils {
public:
	static	void			Draw(BView* target, BPoint at, float value,
								const BBitmap* star);
	static	void			Draw(BView* target, BPoint at, float value);
private:
	static	BReference<SharedBitmap>
							sStarBlueBitmap;
	static	BReference<SharedBitmap>
							sStarGrayBitmap;
};


#endif // RATING_UTILS_H
