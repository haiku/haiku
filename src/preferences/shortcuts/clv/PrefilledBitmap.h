#ifndef PreFilledBitmap_h
#define PreFilledBitmap_h

#include <interface/Bitmap.h>

//Useful until be implements BBitmap::BBitmap(BRect bounds, color_space space, const void *data, 
//										 	  bool acceptsViews, bool needsContiguousMemory)
//or something like it...

class PrefilledBitmap : public BBitmap
{
	public:
		PrefilledBitmap(BRect bounds, color_space space, const void *data, bool acceptsViews,
			bool needsContiguousMemory);
		~PrefilledBitmap();
};

#endif