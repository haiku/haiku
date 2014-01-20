/*
 * Copyright 1999-2009 Jeremy Friesner
 * Copyright 2009-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 *		John Scipione, jscipione@gmail.com
 */


#include "PrefilledBitmap.h"


PrefilledBitmap::PrefilledBitmap(BRect bounds, color_space space,
	const void* data, bool acceptsViews, bool needsContiguousMemory)
	:
	BBitmap(bounds, space, acceptsViews, needsContiguousMemory)
{
	int32 length = ((int32(bounds.right - bounds.left) + 3) / 4) * 4;
	length *= int32(bounds.bottom - bounds.top) + 1;
	SetBits(data, length, 0, space);
}


PrefilledBitmap::~PrefilledBitmap()
{
}
