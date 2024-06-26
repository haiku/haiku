/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "BitmapHolder.h"


BitmapHolder::BitmapHolder(const BBitmap* bitmap)
	:
	fBitmap(bitmap)
{
}


BitmapHolder::~BitmapHolder()
{
	delete fBitmap;
}


const BBitmap*
BitmapHolder::Bitmap() const
{
	return fBitmap;
}
