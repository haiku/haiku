#include <Screen.h>
#include <Bitmap.h>

#include "Bitmaps.h"

void 
ReplaceTransparentColor(BBitmap *bitmap, rgb_color with)
{
//	ASSERT(bitmap->ColorSpace() == B_COLOR_8_BIT); // other color spaces not implemented yet
	
	BScreen screen(B_MAIN_SCREEN_ID);
	uint32 withIndex = screen.IndexForColor(with); 
	
	uchar *bits = (uchar *)bitmap->Bits();
	int32 bitsLength = bitmap->BitsLength();	
	for (int32 index = 0; index < bitsLength; index++) 
		if (bits[index] == B_TRANSPARENT_8_BIT)
			bits[index] = withIndex;
}
