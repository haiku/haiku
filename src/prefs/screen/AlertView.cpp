#include <Bitmap.h>
#include <String.h>
#include <View.h>

#include "AlertView.h"
#include "Bitmaps.h"
#include "Constants.h"
#include "Utility.h"

AlertView::AlertView(BRect frame, char *name)
	: BView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	Count = 8;
	
	fBitmap = new BBitmap(BRect(0, 0, 31, 31), B_COLOR_8_BIT);
	fBitmap->SetBits(BitmapBits, 32 * 32, 0, B_COLOR_8_BIT);
}


void
AlertView::AttachedToWindow()
{
	SetViewColor(greyColor);
}


void
AlertView::Draw(BRect updateRect)
{	
	SetHighColor(darkColor);
	
	FillRect(BRect(0.0, 0.0, 30.0, 100.0));
	
	SetDrawingMode(B_OP_ALPHA);
	
	DrawBitmap(fBitmap, BPoint(18.0, 6.0));
	
	SetDrawingMode(B_OP_OVER);
	
	SetHighColor(blackColor);
	
	SetFont(be_bold_font);
	
	DrawString("Do you wish to keep these settings?", BPoint(60.0, 20.0));
	
	MovePenTo(60.0, 37.0);
	
	fString.Truncate(0);
	
	fString << "Settings will revert in " << Count << " seconds.";
	
	DrawString(fString.String());
}
