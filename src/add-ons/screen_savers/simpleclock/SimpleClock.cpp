/*
 * Copyright (c) 2008-2009 Gerasim Troeglazov (3dEyes**). All Rights Reserved.
 * This file may be used under the terms of the MIT License.
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
	status_t		StartSaver(BView *view, bool preview);
	void			Draw(BView *view, int32 frame);	
private:
	void 			_drawBlock(BView *view, float x, float y, float alpha,
						float size);
	void 			_drawArrow(BView *view, float x0, float y0, float angle,
						float length, float coeff, float width);
	float 			centerX, centerY;
};


extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *message,
	image_id image)
{
	return new Clock(message, image);
}


Clock::Clock(BMessage *message, image_id image)
	:
	BScreenSaver(message, image)
{
	B_TRANSLATE_MARK_SYSTEM_NAME_VOID("SimpleClock");
}


void
Clock::StartConfig(BView *view)
{
	BStringView	*aboutView = new BStringView(BRect(10, 10, 200, 35),
		B_EMPTY_STRING, "Simple Clock");
	aboutView->SetFont(be_bold_font);
	aboutView->SetFontSize(15);
	view->AddChild(aboutView);
	aboutView = new BStringView(BRect(10, 40, 200, 65),
		B_EMPTY_STRING, " Ver 1.0, ©3dEyes**");
	view->AddChild(aboutView);
}


status_t
Clock::StartSaver(BView *view, bool)
{
	SetTickSize(1000000);
	return B_OK;
}


void
Clock::Draw(BView *view, int32)
{
	BScreen screenView;
	BBitmap bufferBitmap(view->Bounds(), screenView.ColorSpace(), true);
	BView offscreenView(view->Bounds(), NULL, 0, 0);
	bufferBitmap.AddChild(&offscreenView);
	bufferBitmap.Lock();

	float width = view->Bounds().Width();
	float height = view->Bounds().Height();
	float zoom = (height / 1024.0) * 0.65;
	
	time_t timeInfo;
	time(&timeInfo);
	struct tm *nowTime = localtime(&timeInfo);

	float secondVal = nowTime->tm_sec;
	float minuteVal = nowTime->tm_min + (secondVal / 60.0);
	float hourVal = nowTime->tm_hour + (minuteVal / 60.0);

	offscreenView.SetHighColor(0, 0, 0);
	offscreenView.SetLowColor(0, 0, 0);
	offscreenView.FillRect(offscreenView.Bounds());

	offscreenView.SetHighColor(200, 200, 200);
	
	centerX = width / 2.0;
	centerY = height / 2.0;

	float markAngle = 0;
	float markRadius = 510.0 * zoom;

	for(int mark = 0; mark < 60; mark++, markAngle += (2 * M_PI) / 60) {
		float x = centerX + markRadius * cos(markAngle);
		float y = centerY + markRadius * sin(markAngle);
		_drawBlock(&offscreenView, x, y, markAngle, 14.0 * zoom);
	}

	offscreenView.SetHighColor(255, 255, 255);

	markAngle = 0;
	markRadius = 500.0 * zoom;
	
	for (int mark = 0; mark < 12; mark++, markAngle += (2 * M_PI) / 12) {
		float x = centerX + markRadius * cos(markAngle);
		float y = centerY + markRadius * sin(markAngle);
		_drawBlock(&offscreenView, x, y, markAngle, 32 * zoom);
	}

	offscreenView.SetHighColor(255, 255, 255);
	_drawArrow(&offscreenView, centerX, centerY,
		((2 * M_PI / 60) * minuteVal) - (M_PI / 2), 220 * zoom, 1, 8 * zoom);

	_drawArrow(&offscreenView, centerX, centerY,
		((2 * M_PI / 12) * hourVal) - (M_PI / 2), 140 * zoom, 1, 14 * zoom);
	offscreenView.FillEllipse(BPoint(centerX, centerY),
		24 * zoom, 24 * zoom);
	
	offscreenView.SetHighColor(250, 20, 20);
	_drawArrow(&offscreenView, centerX, centerY,
		((2 * M_PI / 60) * secondVal) - (M_PI / 2), 240 * zoom, 1, 4 * zoom);
	offscreenView.FillEllipse(BPoint(centerX, centerY),
		20 * zoom, 20 * zoom);

	offscreenView.Sync();
	bufferBitmap.Unlock();
	view->DrawBitmap(&bufferBitmap);
	bufferBitmap.RemoveChild(&offscreenView);
}


void
Clock::_drawBlock(BView *view, float x, float y, float alpha, float size)
{
	float blockAngles[4] = {alpha - (M_PI / 12), alpha + (M_PI / 12),
		alpha + M_PI - (M_PI / 12), alpha + M_PI + (M_PI / 12)};
	
	BPoint blockPoints[4];
	for (int index = 0; index < 4; index++) {
		blockPoints[index].x = x + size * cos(blockAngles[index]);
		blockPoints[index].y = y + size * sin(blockAngles[index]);
	}
	view->FillPolygon(&blockPoints[0], 4);
}


void
Clock::_drawArrow(BView *view, float x0, float y0, float angle, float length,
	float coeff, float width)
{
	float alpha = width / length;

	float x = x0 + length * cos(angle);
	float y = y0 + length * sin(angle);

	float size = length * coeff;

	float blockAngles[4] = {angle - alpha, angle + alpha,
		angle + M_PI - alpha, angle + M_PI + alpha};

	BPoint blockPoints[4];
	for(int index = 0; index < 4; index++) {
		blockPoints[index].x = x + size * cos(blockAngles[index]);
		blockPoints[index].y = y + size * sin(blockAngles[index]);
	}
	view->FillPolygon(&blockPoints[0], 4);
}
