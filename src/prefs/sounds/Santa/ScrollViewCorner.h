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
//If you have a BScrollView with horizontal and vertical sliders that isn't 
//seated to the lower-right corner of a B_DOCUMENT_WINDOW, there's a "hole"
//between the sliders that needs to be filled.  You can use this to fill it.
//In general, it looks best to set the ScrollViewCorner color to 
//BeInactiveControlGrey if the vertical BScrollBar is inactive, and the color
//to BeBackgroundGrey if the vertical BScrollBar is active.  Have a look at
//Demo3 of ColumnListView to see what I mean if this is unclear.


#ifndef _SGB_SCROLL_VIEW_CORNER_H_
#define _SGB_SCROLL_VIEW_CORNER_H_


//******************************************************************************************************
//**** SYSTEM HEADER FILES
//******************************************************************************************************
#include <View.h>


//******************************************************************************************************
//**** CLASS DECLARATIONS
//******************************************************************************************************
class ScrollViewCorner : public BView
{
	public:
		ScrollViewCorner(float Left,float Top);
		~ScrollViewCorner();
		virtual void Draw(BRect Update);
};


#endif
