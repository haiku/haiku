#include <Alert.h>
#include <Bitmap.h>
#include <Debug.h>
#include <Screen.h>
#include <String.h>
#include <SupportDefs.h>
#include <Window.h>

#include <stdio.h>

#include "AnalogClock.h"
#include "Bitmaps.h"
#include "TimeMessages.h"

TOffscreen::TOffscreen(BRect frame, char *name)
	: BView(frame, name, B_NOT_RESIZABLE, B_WILL_DRAW)
{
	f_bitmap = new BBitmap(BRect(0, 0, kClockFaceWidth, kClockFaceHeight), kClockFaceColorSpace);
	f_bitmap->SetBits(kClockFaceBits, (kClockFaceWidth +1) *(kClockFaceHeight +1), 0, kClockFaceColorSpace);
	ReplaceTransparentColor(f_bitmap, ui_color(B_PANEL_BACKGROUND_COLOR));
	
	float counter;
	short index = 0;
	float x,y;
	float mRadius = 30;
	float hRadius = 20;
	f_Offset = 41;
	//
	// Generate minutes points array
	//
	for (counter = 90; counter >= 0; counter -= 6,index++) {
		x = mRadius * cos(((360 - counter)/180.0) * 3.1415);
		x += f_Offset;
		y = mRadius * sin(((360 - counter)/180.0) * 3.1415);
		y += f_Offset;
		f_MinutePoints[index].Set(x,y);
		x = hRadius * cos(((360 - counter)/180.0) * 3.1415);
		x += f_Offset;
		y = hRadius * sin(((360 - counter)/180.0) * 3.1415);
		y += f_Offset;
		f_HourPoints[index].Set(x,y);
	}
	for (counter = 354; counter > 90; counter -= 6,index++) {
		x = mRadius * cos(((360 - counter)/180.0) * 3.1415);
		x += f_Offset;
		y = mRadius * sin(((360 - counter)/180.0) * 3.1415);
		y += f_Offset;
		f_MinutePoints[index].Set(x,y);
		x = hRadius * cos(((360 - counter)/180.0) * 3.1415);
		x += f_Offset;
		y = hRadius * sin(((360 - counter)/180.0) * 3.1415);
		y += f_Offset;
		f_HourPoints[index].Set(x,y);
	}
	BRect bounds(Bounds());
	f_center = BPoint(bounds.Width()/2.0, bounds.Height()/2.0);
	f_offset = BPoint((kClockFaceWidth-1)/2, (kClockFaceHeight-1)/2);
}

TOffscreen::~TOffscreen()
{
}

void
TOffscreen::AttachedToWindow()
{
	SetViewColor(200, 20, 20);
	SetFontSize(18);
	SetFont(be_plain_font);
}

void
TOffscreen::DrawX()
{
	short hours;
	ASSERT(Window());
	if (Window()->Lock())
	{
		DrawBitmap(f_bitmap, BPoint(0, 0));
	
		SetHighColor(0, 0, 0);
		hours = f_Hours;
		if (hours>= 12)
			hours -= 12;
		hours *= 5;
		hours += (f_Minutes/12);
		
		StrokeLine(f_center, f_HourPoints[hours]);
		SetDrawingMode(B_OP_COPY);
		StrokeLine(f_center, f_MinutePoints[f_Minutes]);
		SetHighColor(tint_color(HighColor(), B_LIGHTEN_1_TINT));
		StrokeLine(f_center, f_MinutePoints[f_Seconds]);
		Sync();
		Window()->Unlock();
	}
}

BPoint
TOffscreen::Position()
{
	return (f_center -f_offset);
}


TAnalogClock::TAnalogClock(BRect frame, const char *name, uint32 resizingmode, uint32 flags)
	: BView(frame, name, resizingmode, flags)
	, f_bitmap(NULL)
	, f_offscreen(NULL)
{ 
	BRect bounds(0, 0, kClockFaceWidth -1, kClockFaceHeight -2);
	f_offscreen = new TOffscreen(bounds, "offscreen");
	f_bitmap = new BBitmap(bounds, B_COLOR_8_BIT, TRUE);
	f_bitmap->Lock();
	f_bitmap->AddChild(f_offscreen);
	f_bitmap->Unlock();
	f_offscreen->DrawX();
	
	BRect rect(Bounds());
	float x = (rect.Width()/2.0) -(bounds.Width()/2.0) -2;
	float y = (rect.Height()/2.0) -(bounds.Height()/2.0) -1;
		
	f_drawpt.Set(x, y);
}

TAnalogClock::~TAnalogClock()
{ 
	delete f_bitmap;
}

void
TAnalogClock::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());
}

void
TAnalogClock::Draw(BRect updaterect)
{
	time_t current;
	ASSERT(f_bitmap);
	ASSERT(f_offscreen);
	
	bool locked = f_bitmap->Lock();
	ASSERT(locked);
	f_offscreen->DrawX();
	DrawBitmap(f_bitmap, f_drawpt);
	f_bitmap->Unlock();
	
	current = time(0);
}

void
TAnalogClock::Update(tm *atm)
{
	if ((atm->tm_sec != f_offscreen->f_Seconds)
		|| (atm->tm_min != f_offscreen->f_Minutes))
	{
		f_offscreen->f_Hours = atm->tm_hour;
		f_offscreen->f_Minutes = atm->tm_min;
		f_offscreen->f_Seconds = atm->tm_sec;
		Draw(Bounds());
	}
}
