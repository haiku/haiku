#include <Bitmap.h>
#include <String.h>
#include <View.h>

#include "AlertView.h"
#include "Bitmaps.h"

AlertView::AlertView(BRect frame, char *name)
	: BView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	Count = 10;
	
	fBitmap = new BBitmap(BRect(0, 0, 31, 31), B_COLOR_8_BIT);
	fBitmap->SetBits(BitmapBits, 32 * 32, 0, B_COLOR_8_BIT);
}

void AlertView::AttachedToWindow()
{
	rgb_color greyColor = {216, 216, 216, 255};
	SetViewColor(greyColor);
}

void AlertView::Draw(BRect updateRect)
{
	rgb_color darkColor = {184, 184, 184, 255};
	rgb_color blackColor = {0, 0, 0, 255};
	
	SetHighColor(darkColor);
	
	FillRect(BRect(0.0, 0.0, 30.0, 100.0));
	
	SetDrawingMode(B_OP_ALPHA);
	
	DrawBitmap(fBitmap, BPoint(18.0, 6.0));
	
	SetDrawingMode(B_OP_OVER);
	
	SetHighColor(blackColor);
	
	SetFont(be_bold_font);
	
	MovePenTo(60.0, 20.0);
	
	DrawString("Do you wish to keep these settings?");
	
	MovePenTo(60.0, 37.0);
	
	fString.Truncate(0);
	
	fString << "Settings will revert in " << Count << " seconds.";
	
	DrawString(fString.String());
}
