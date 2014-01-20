/*
 * Copyright 1999-2009 Jeremy Friesner
 * Copyright 2009-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 *		John Scipione, jscipione@gmail.com
 */
#ifndef PREFILLED_BITMAP_H
#define PREFILLED_BITMAP_H


#include <interface/Bitmap.h>


class PrefilledBitmap : public BBitmap
{
public:
								PrefilledBitmap(BRect bounds,
									color_space space, const void* data,
									bool acceptsViews,
									bool needsContiguousMemory);
	virtual						~PrefilledBitmap();
};


#endif	// PREFILLED_BITMAP_H
