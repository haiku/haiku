#include <Alert.h>
#include <Bitmap.h>
#include <Debug.h>
#include <Window.h>

#include "AnalogClock.h"
#include "Bitmaps.h"
#include "TimeMessages.h"

const BRect kClockRect(0, 0, kClockFaceWidth -1, kClockFaceHeight -1);
const BRect kCenterRect(0, 0, kCenterWidth -1, kCenterHeight -1);
const BRect kCapRect(0, 0, kCapWidth -1, kCapHeight -1);


TOffscreen::TOffscreen(BRect frame, const char *name)
	: BView(frame, name, B_NOT_RESIZABLE, B_WILL_DRAW)
{
	f_bitmap = new BBitmap(kClockRect, kClockFaceColorSpace);
	f_bitmap->SetBits(kClockFaceBits, (kClockFaceWidth) *(kClockFaceHeight +3), 0, kClockFaceColorSpace);

	ReplaceTransparentColor(f_bitmap, ui_color(B_PANEL_BACKGROUND_COLOR));

	f_centerbmp = new BBitmap(kCenterRect, kCenterColorSpace);
	f_centerbmp->SetBits(kCenterBits, (kCenterWidth) *(kCenterHeight +1), 0, kCenterColorSpace);

	f_capbmp = new BBitmap(kCapRect, kCapColorSpace);
	f_capbmp->SetBits(kCapBits, (kCapWidth +1) *(kCapHeight +1) +1, 0, kCapColorSpace);
	
	f_center = BPoint(42, 42);

	float counter;
	short index;
	float x, y, mRadius, hRadius;
	
	mRadius = f_center.x -12;
	hRadius = mRadius -10;
	index = 0;
	
	//
	// Generate minutes/hours points array
	//
	for (counter = 90; counter >= 0; counter -= 6,index++) {
		x = mRadius * cos(((360 - counter)/180.0) * 3.1415);
		x += f_center.x;
		y = mRadius * sin(((360 - counter)/180.0) * 3.1415);
		y += f_center.x;
		f_MinutePoints[index].Set(x,y);
		x = hRadius * cos(((360 - counter)/180.0) * 3.1415);
		x += f_center.x;
		y = hRadius * sin(((360 - counter)/180.0) * 3.1415);
		y += f_center.x;
		f_HourPoints[index].Set(x,y);
	}
	for (counter = 354; counter > 90; counter -= 6,index++) {
		x = mRadius * cos(((360 - counter)/180.0) * 3.1415);
		x += f_center.x;
		y = mRadius * sin(((360 - counter)/180.0) * 3.1415);
		y += f_center.x;
		f_MinutePoints[index].Set(x,y);
		x = hRadius * cos(((360 - counter)/180.0) * 3.1415);
		x += f_center.x;
		y = hRadius * sin(((360 - counter)/180.0) * 3.1415);
		y += f_center.x;
		f_HourPoints[index].Set(x,y);
	}
}


TOffscreen::~TOffscreen()
{	
	delete f_bitmap;
}


void
TOffscreen::DrawX()
{
	if (Window()->Lock()) {
	
		// draw clockface
		SetDrawingMode(B_OP_COPY);
		DrawBitmap(f_bitmap, BPoint(0, 0));

		SetHighColor(0, 0, 0, 255);
		
		short hours = f_Hours;
		if (hours>= 12)
			hours -= 12;
			
		hours *= 5;
		hours += (f_Minutes / 12);
		
		// draw center hub
		SetDrawingMode(B_OP_OVER);
		DrawBitmap(f_centerbmp, f_center -BPoint(kCenterWidth/2.0, kCenterHeight/2.0));

		// draw hands
		StrokeLine(f_center, f_HourPoints[hours]);
		StrokeLine(f_center, f_MinutePoints[f_Minutes]);
		SetHighColor(tint_color(HighColor(), B_LIGHTEN_1_TINT));
		StrokeLine(f_center, f_MinutePoints[f_Seconds]);

		// draw center cap
		DrawBitmap(f_capbmp, f_center -BPoint(kCapWidth/2.0, kCapHeight/2.0));
		
 		Sync();
		Window()->Unlock();
	}
}



/*=====> TAnalogClock <=====*/

TAnalogClock::TAnalogClock(BRect frame, const char *name, uint32 resizingmode, uint32 flags)
	: BView(frame, name, resizingmode, flags|B_DRAW_ON_CHILDREN)
{
	InitView(frame);
}


TAnalogClock::~TAnalogClock()
{
	delete f_bitmap;
}


void
TAnalogClock::InitView(BRect rect)
{

#if 1
	f_offscreen = new TOffscreen(kClockRect, "offscreen");
	f_bitmap = new BBitmap(kClockRect, B_COLOR_8_BIT, true);
	f_bitmap->Lock();
	f_bitmap->AddChild(f_offscreen);
	f_bitmap->Unlock();
	f_offscreen->DrawX();
#endif
	
	// offscreen clock is kClockFaceWidth by kClockFaceHeight
	// which might be smaller then TAnalogClock frame so "center" it.
	f_drawpt = BPoint(rect.Width()/2.0 -(kClockFaceWidth/2.0), 
			  rect.Height()/2.0 -(kClockFaceHeight/2.0));	
}


void
TAnalogClock::AttachedToWindow()
{
	if (Parent()) {
		SetViewColor(Parent()->ViewColor());
	}
}


void
TAnalogClock::MessageReceived(BMessage *message)
{
	int32 change;
	switch(message->what) {
		case B_OBSERVER_NOTICE_CHANGE:
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &change);
			switch (change) {
				case OB_TM_CHANGED:
				{
					int32 hour = 0;
					int32 minute = 0;
					int32 second = 0;
					if (message->FindInt32("hour", &hour) == B_OK
					 && message->FindInt32("minute", &minute) == B_OK
					 && message->FindInt32("second", &second) == B_OK)
						SetTo(hour, minute, second);
					break;
				}
				default:
					BView::MessageReceived(message);
					break;
			}
		break;
		default:
			BView::MessageReceived(message);
			break;
	}
}


void
TAnalogClock::Draw(BRect updaterect)
{
	ASSERT(f_offscreen);
	ASSERT(f_bitmap);
	SetHighColor(0, 100, 10);
	bool b = f_bitmap->Lock();
	ASSERT(b);
	f_offscreen->DrawX();
	DrawBitmap(f_bitmap, f_drawpt);
	f_bitmap->Unlock();

}


void
TAnalogClock::SetTo(int32 hour, int32 minute, int32 second)
{
	if (f_offscreen->f_Seconds != second
	 || f_offscreen->f_Minutes != minute) {
		f_offscreen->f_Hours = hour;
		f_offscreen->f_Minutes = minute;
		f_offscreen->f_Seconds = second;
		Draw(Bounds());
	}
}
