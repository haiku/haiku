#include <View.h>

#include "RefreshView.h"

RefreshView::RefreshView(BRect rect, char *name)
	: BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
}


void
RefreshView::AttachedToWindow()
{
	rgb_color grey = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetViewColor(grey);
}


void
RefreshView::Draw(BRect updateRect)
{
	rgb_color dark3 = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_3_TINT);
	rgb_color white = { 255, 255, 255, 255 };
	rgb_color black = { 0, 0, 0, 255 };
	
	BRect bounds(Bounds());
	SetHighColor(white);
	
	StrokeLine(BPoint(bounds.left, bounds.top), BPoint(bounds.right, bounds.top));
	StrokeLine(BPoint(bounds.left, bounds.top), BPoint(bounds.left, bounds.bottom));
	
	SetHighColor(dark3);
	
	StrokeLine(BPoint(bounds.left, bounds.bottom), BPoint(bounds.right, bounds.bottom));
	StrokeLine(BPoint(bounds.right, bounds.bottom), BPoint(bounds.right, bounds.top));
	
	SetHighColor(black);
	SetLowColor(ViewColor());
	
	DrawString("Type or use the left and right arrow keys.", BPoint(10.0, 23.0));
}
