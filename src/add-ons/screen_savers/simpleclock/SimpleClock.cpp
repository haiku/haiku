/*
**
** A simple analog clock screensaver.
**
** Version: 0.1
**
** Copyright (c) 2008-2009 Gerasim Troeglazov (3dEyes**). All Rights Reserved.
** This file may be used under the terms of the MIT License.
*/


#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <Bitmap.h>
#include <Catalog.h>
#include <Polygon.h>
#include <Screen.h>
#include <ScreenSaver.h>
#include <StringView.h>
#include <View.h>


class Clock : public BScreenSaver
{
public:
					Clock(BMessage *message, image_id id);
	void			StartConfig(BView *view);
	status_t		StartSaver(BView *v, bool preview);
	void			Draw(BView *v, int32 frame);
	BStringView		*tview;
private:
	void 			DrawBlock(BView *view, float x, float y, float a, float size);
	void 			DrawArrow(BView *view, float xc, float yc, float a, float len, float k,float width);
	time_t			tmptodaytime;
	float			todaysecond , todayminute , todayhour;
	struct tm		*TodayTime;
	BRect r;
};


extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *message, image_id image)
{
	return new Clock(message, image);
}


Clock::Clock(BMessage *message, image_id image)
	:
	BScreenSaver(message, image)
{
	B_TRANSLATE_MARK_SYSTEM_NAME_VOID("SimpleClock");
}


void Clock::StartConfig(BView *view)
{
	tview = new BStringView(BRect(10, 10, 200, 35), B_EMPTY_STRING, "Simple Clock");
	tview->SetFont(be_bold_font);
	tview->SetFontSize(15);
	view->AddChild(tview);
	view->AddChild(new BStringView(BRect(10, 40, 200, 65), B_EMPTY_STRING, " Ver 0.1, ©3dEyes**"));
}

status_t Clock::StartSaver(BView *view, bool preview)
{
	SetTickSize(1000000);
	return B_OK;
}

void Clock::DrawBlock(BView *view, float x, float y, float a, float size)
{
	float angles[4]={a-(M_PI/12),a+(M_PI/12),a+(M_PI)-(M_PI/12),a+(M_PI)+(M_PI/12)};
	BPoint points[4];
	for(int i=0;i<4;i++) {
		points[i].x= x+size*cos(angles[i]);
		points[i].y= y+size*sin(angles[i]);
	}
	view->FillPolygon(&points[0],4);

}

void Clock::DrawArrow(BView *view, float xc, float yc, float a, float len, float k, float width)
{
	float g = width/len;

	float x = xc+(len)*cos(a);
	float y = yc+(len)*sin(a);

	float size = len*k;

	float angles[4]={a-g,a+g,a+(M_PI)-g,a+(M_PI)+g};

	BPoint points[4];
	for(int i=0;i<4;i++) {
		points[i].x= x+size*cos(angles[i]);
		points[i].y= y+size*sin(angles[i]);
	}
	view->FillPolygon(&points[0],4);
}

void Clock::Draw(BView *view, int32)
{
	BScreen screen;
	BBitmap buffer(view->Bounds(), screen.ColorSpace(), true);
	BView offscreen(view->Bounds(), NULL, 0, 0);
	buffer.AddChild(&offscreen);
	buffer.Lock();

	int n;
	float a,R;
	float width = view->Bounds().Width();
	float height = view->Bounds().Height();
	float zoom = (height/1024) * 0.85;

	time(&tmptodaytime);
	TodayTime = localtime(&tmptodaytime);

	todaysecond = TodayTime->tm_sec;
	todayminute = TodayTime->tm_min + (todaysecond/60.0);
	todayhour   = TodayTime->tm_hour + (todayminute/60.0);

	rgb_color bg_color = {0,0,0};
	offscreen.SetHighColor(bg_color);
	offscreen.SetLowColor(bg_color);
	offscreen.FillRect(offscreen.Bounds());

	offscreen.SetHighColor(200,200,200);

	for(n=0,a=0,R=510*zoom;n<60;n++,a+=(2*M_PI)/60) {
		float x = width/2 + R * cos(a);
		float y = height/2 + R * sin(a);
		DrawBlock(&offscreen,x,y,a,14*zoom);
	}

	offscreen.SetHighColor(255,255,255);

	for(n=0,a=0,R=500*zoom;n<12;n++,a+=(2*M_PI)/12) {
		float x = width/2 + R * cos(a);
		float y = height/2 + R * sin(a);
		DrawBlock(&offscreen,x,y,a,32*zoom);
	}

	offscreen.SetHighColor(255,255,255);
	DrawArrow(&offscreen, width/2,height/2, ( ((2*M_PI)/60) * todayminute) - (M_PI/2), 220*zoom, 1, 8*zoom);
	DrawArrow(&offscreen, width/2,height/2, ( ((2*M_PI)/12) * todayhour) - (M_PI/2), 140*zoom, 1, 14*zoom);
	offscreen.FillEllipse(BPoint(width/2,height/2),24*zoom,24*zoom);
	offscreen.SetHighColor(250,20,20);
	DrawArrow(&offscreen, width/2,height/2, ( ((2*M_PI)/60) * todaysecond) - (M_PI/2), 240*zoom, 1, 4*zoom);
	offscreen.FillEllipse(BPoint(width/2,height/2),20*zoom,20*zoom);

	offscreen.Sync();
	buffer.Unlock();
	view->DrawBitmap(&buffer);
	buffer.RemoveChild(&offscreen);
}
