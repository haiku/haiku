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

#include "PrefilledBitmap.h"

PrefilledBitmap::PrefilledBitmap(BRect bounds, color_space space, const void *data, int32 length,
	bool acceptsViews, bool needsContiguousMemory)
: BBitmap(bounds, space, acceptsViews, needsContiguousMemory)
{
	if(length == 0)
	{
		if(space == B_CMAP8)
			length = ((int32(bounds.right-bounds.left+1)+3)&0xFFFFFFFC)*int32(bounds.bottom-bounds.top+1);
		else if(space == B_RGB32)
			length = int32(bounds.right-bounds.left+1)*int32(bounds.bottom-bounds.top+1)*3;
	}
	SetBits(data, length, 0, space);
}


PrefilledBitmap::~PrefilledBitmap()
{ }