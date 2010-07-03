#include "Rasterizer.h"

#include <stdio.h>


Rasterizer::Rasterizer(Halftone* halftone)
	:
	fHalftone(halftone),
	fIndex(-1)
{
	fBounds.bottom = -2;
}


Rasterizer::~Rasterizer()
{
}


bool 
Rasterizer::SetBitmap(int x, int y, BBitmap* bitmap, int pageHeight)
{
	fX = x; 
	fY = y;
	
	BRect bounds = bitmap->Bounds();

	fBounds.left = (int)bounds.left;
	fBounds.top = (int)bounds.top;
	fBounds.right = (int)bounds.right;
	fBounds.bottom = (int)bounds.bottom;

	int height = fBounds.bottom - fBounds.top + 1;	

	if (y + height > pageHeight) {
		height = pageHeight - y;
		fBounds.bottom = fBounds.top + height - 1;
	}
		
	if (!get_valid_rect(bitmap, &fBounds))
		return false;

	fWidth = fBounds.right - fBounds.left + 1;
	fHeight = fBounds.bottom - fBounds.top + 1;	
	
	fBPR = bitmap->BytesPerRow();
	fBits = (uchar*)bitmap->Bits();
	// offset to top, left point of rect
	fBits += fBounds.top * fBPR + fBounds.left * 4;	
	
	// XXX why not fX += ...?
	fX = fBounds.left;
	fY += fBounds.top;
	fIndex = fBounds.top;
	return true;
}


bool 
Rasterizer::HasNextLine()
{
	return fIndex <= fBounds.bottom;
}


const void* 
Rasterizer::RasterizeNextLine()
{
	if (!HasNextLine())
		return NULL;

	const void* result;
	result = RasterizeLine(fX, fY, (const ColorRGB32Little*)fBits);
	fBits += fBPR;
	fY ++;
	fIndex ++;
	return result;
}


void
Rasterizer::RasterizeBitmap()
{
	while (HasNextLine())
		RasterizeNextLine();
}
