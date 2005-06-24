#include "RefreshView.h"

RefreshView::RefreshView(BRect rect, char *name)
	: BBox(rect, name)
{
}


void
RefreshView::Draw(BRect updateRect)
{
	rgb_color black = { 0, 0, 0, 255 };
	
	BBox::Draw(updateRect);
	
	SetHighColor(black);
	SetLowColor(ViewColor());
	
	DrawString("Type or use the left and right arrow keys.", BPoint(10.0, 23.0));
}
