//*** LICENSE ***
//ColumnListView, its associated classes and source code, and the other components of Santa's Gift Bag are
//being made publicly available and free to use in freeware and shareware products with a price under $25
//(I believe that shareware should be cheap). For overpriced shareware (hehehe) or commercial products,
//please contact me to negotiate a fee for use. After all, I did work hard on this class and invested a lot
//of time into it. That being said, DON'T WORRY I don't want much. It totally depends on the sort of project
//you're working on and how much you expect to make off it. If someone makes money off my work, I'd like to
//get at least a little something.  If any of the components of Santa's Gift Bag are is used in a shareware
//or commercial product, I get a free copy.  The source is made available so that you can improve and extend
//it as you need. In general it is best to customize your ColumnListView through inheritance, so that you
//can take advantage of enhancements and bug fixes as they become available. Feel free to distribute the 
//ColumnListView source, including modified versions, but keep this documentation and license with it.

//*** DESCRIPTION ***
//Useful until be implements BBitmap::BBitmap(BRect bounds, color_space space, const void *data, 
//										 	  bool acceptsViews, bool needsContiguousMemory)
//or something like it...


#ifndef _SGB_PREFILLED_BITMAP_H_
#define _SGB_PREFILLED_BITMAP_H_


//******************************************************************************************************
//**** SYSTEM HEADER FILES
//******************************************************************************************************
#include <Bitmap.h>


//******************************************************************************************************
//**** CLASS DECLARATIONS
//******************************************************************************************************
class PrefilledBitmap : public BBitmap
{
	public:
		PrefilledBitmap(BRect bounds, color_space space, const void *data, int32 length = 0,
			bool acceptsViews = false, bool needsContiguousMemory = false);
			//length can be automatically calculated for B_CMAP8 and B_RGB32
		~PrefilledBitmap();
};

#endif
