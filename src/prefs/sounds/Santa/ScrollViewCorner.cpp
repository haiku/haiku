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

#include <ScrollBar.h>
#include "Colors.h"
#include "ScrollViewCorner.h"


ScrollViewCorner::ScrollViewCorner(float Left,float Top)
: BView(BRect(Left,Top,Left+B_V_SCROLL_BAR_WIDTH,Top+B_H_SCROLL_BAR_HEIGHT),NULL,B_FOLLOW_RIGHT |
	B_FOLLOW_BOTTOM,B_WILL_DRAW)
{
	SetHighColor(BeShadow);
	SetViewColor(BeInactiveGrey);
}


ScrollViewCorner::~ScrollViewCorner()
{ }


void ScrollViewCorner::Draw(BRect Update)
{
	if(Update.bottom >= B_H_SCROLL_BAR_HEIGHT)
		StrokeLine(BPoint(0.0,B_H_SCROLL_BAR_HEIGHT),BPoint(B_V_SCROLL_BAR_WIDTH,B_H_SCROLL_BAR_HEIGHT));
	if(Update.right >= B_V_SCROLL_BAR_WIDTH)
		StrokeLine(BPoint(B_V_SCROLL_BAR_WIDTH,0.0),
			BPoint(B_V_SCROLL_BAR_WIDTH,B_H_SCROLL_BAR_HEIGHT-1.0));
}


