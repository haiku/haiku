#include <Bitmap.h>
#include <String.h>
#include <View.h>

#include "AlertView.h"
#include "Bitmaps.h"
#include "Constants.h"

AlertView::AlertView(BRect frame, char *name)
	:BView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	count = 8;
	
	fBitmap = new BBitmap(BRect(0, 0, 31, 31), B_COLOR_8_BIT);
	fBitmap->SetBits(bitmapBits, 32 * 32, 0, B_COLOR_8_BIT);
}


void
AlertView::AttachedToWindow()
{
	rgb_color grey = ui_color(B_PANEL_BACKGROUND_COLOR);
	
	SetViewColor(grey);
}


void
AlertView::Draw(BRect updateRect)
{	
	rgb_color dark = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_1_TINT);
	rgb_color black = { 0, 0, 0, 255 };
	
	SetHighColor(dark);	
	FillRect(BRect(0.0, 0.0, 30.0, 100.0));
	
	SetDrawingMode(B_OP_ALPHA);
	DrawBitmap(fBitmap, BPoint(18.0, 6.0));
	
	SetDrawingMode(B_OP_OVER);
	SetHighColor(black);
	SetFont(be_bold_font);
	
	DrawString("Do you wish to keep these settings?", BPoint(60.0, 20.0));
	
	fString.Truncate(0);
	fString << "Settings will revert in " << count << " seconds.";
	
	DrawString(fString.String(), BPoint(60, 37));
}
