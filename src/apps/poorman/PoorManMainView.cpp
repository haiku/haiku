/* PoorManMainView.cpp
 *
 *	Philip Harrison
 *	Started: 5/13/2004
 *	Version: 0.1
 */
 

#include "PoorManMainView.h"

PoorManMainView::PoorManMainView(BRect rect, char *name)
	: BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
}

void 
PoorManMainView::AttachedToWindow()
{
	SetPenSize(1);
}

void 
PoorManMainView::Draw(BRect updateRect)
{
	BRect tempRect;
	tempRect = updateRect;

	StrokeRect(tempRect, B_SOLID_HIGH);
}
