/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <Debug.h>
#include <Bitmap.h>

#include "ConvertBitmap.h"

status_t ConvertBitmap_YCbCr422_to_RGB32(BBitmap *dst, const BBitmap *src);

status_t
ConvertBitmap(BBitmap *dst, const BBitmap *src)
{
	if (dst->Bounds() != src->Bounds())
		return B_BAD_VALUE;
	
	if (dst->ColorSpace() == src->ColorSpace() 
		&& dst->BytesPerRow() == src->BytesPerRow() ) {
		memcpy(dst->Bits(), src->Bits(), src->BitsLength());
		return B_OK;
	}

	if (src->ColorSpace() == B_YCbCr422 && dst->ColorSpace() == B_RGB32)
		return ConvertBitmap_YCbCr422_to_RGB32(dst, src);

	return B_ERROR;
}


status_t
ConvertBitmap_YCbCr422_to_RGB32(BBitmap *dst, const BBitmap *src)
{
	return B_ERROR;
}
