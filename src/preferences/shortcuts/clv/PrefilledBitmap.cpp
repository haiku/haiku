#include "PrefilledBitmap.h"

PrefilledBitmap::PrefilledBitmap(BRect bounds, color_space space, const void *data, bool acceptsViews,
	bool needsContiguousMemory)
: BBitmap(bounds, space, acceptsViews, needsContiguousMemory)
{
	int32 length = ((int32(bounds.right-bounds.left)+3) / 4) * 4;
	length *= int32(bounds.bottom-bounds.top)+1;
	SetBits(data, length, 0, space);
}


PrefilledBitmap::~PrefilledBitmap()
{ }