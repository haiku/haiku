#include <View.h>

#include "RefreshView.h"
#include "Utility.h"

RefreshView::RefreshView(BRect rect, char *name)
	: BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
}


void
RefreshView::AttachedToWindow()
{
	SetViewColor(greyColor);
}


void
RefreshView::Draw(BRect updateRect)
{
	rgb_color darkColor = { 128, 128, 128, 255 };
	
	BRect bounds(Bounds());
	SetHighColor(whiteColor);
	
	StrokeLine(BPoint(bounds.left, bounds.top), BPoint(bounds.right, bounds.top));
	StrokeLine(BPoint(bounds.left, bounds.top), BPoint(bounds.left, bounds.bottom));
	
	SetHighColor(darkColor);
	
	StrokeLine(BPoint(bounds.left, bounds.bottom), BPoint(bounds.right, bounds.bottom));
	StrokeLine(BPoint(bounds.right, bounds.bottom), BPoint(bounds.right, bounds.top));
	
	SetHighColor(blackColor);
	SetLowColor(ViewColor());
	
	DrawString("Type or use the left and right arrow keys.", BPoint(10.0, 23.0));
}
