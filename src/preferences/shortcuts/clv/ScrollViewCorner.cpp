#include "Colors.h"
#include "ScrollViewCorner.h"

#include <InterfaceKit.h>

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


