#include <View.h>

#include "ScreenView.h"
#include "Utility.h"

ScreenView::ScreenView(BRect rect, char *name)
	: BView(rect, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
}


void
ScreenView::AttachedToWindow()
{
	SetViewColor(greyColor);
}


void
ScreenView::Draw(BRect updateRect)
{
	rgb_color darkColor = {128, 128, 128, 255};
	
	SetHighColor(whiteColor);
	
	StrokeLine(BPoint(Bounds().left, Bounds().top), BPoint(Bounds().right, Bounds().top));
	StrokeLine(BPoint(Bounds().left, Bounds().top), BPoint(Bounds().left, Bounds().bottom));
	
	SetHighColor(darkColor);
	
	StrokeLine(BPoint(Bounds().left, Bounds().bottom), BPoint(Bounds().right, Bounds().bottom));
	StrokeLine(BPoint(Bounds().right, Bounds().bottom), BPoint(Bounds().right, Bounds().top));
	
	SetHighColor(blackColor);
}
