/*
 * Copyright 2003-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonas Sundström, jonas@kirilla.com
 */


#include "ZipOMaticActivity.h"

#include <stdio.h>


Activity::Activity(const char* name)
	:
	BView(name, B_WILL_DRAW | B_FRAME_EVENTS),
	fIsRunning(false),
	fBitmap(NULL)
{
	fPattern.data[0] = 0x0f;
	fPattern.data[1] = 0x1e;
	fPattern.data[2] = 0x3c;
	fPattern.data[3] = 0x78;
	fPattern.data[4] = 0xf0;
	fPattern.data[5] = 0xe1;
	fPattern.data[6] = 0xc3;
	fPattern.data[7] = 0x87;

	SetExplicitMinSize(BSize(17, 17));
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 17));
};


Activity::~Activity()
{
	delete fBitmap;
}


void 
Activity::AllAttached()
{
	SetViewColor(B_TRANSPARENT_COLOR);
	_CreateBitmap();
}


void 
Activity::Start()
{
	fIsRunning = true;
	Window()->SetPulseRate(100000);
	SetFlags(Flags() | B_PULSE_NEEDED);
	Invalidate();
}


void 
Activity::Pause()
{
	Window()->SetPulseRate(500000);
	SetFlags(Flags() & (~B_PULSE_NEEDED));
	Invalidate();
}


void 
Activity::Stop()
{
	fIsRunning = false;
	Window()->SetPulseRate(500000);
	SetFlags(Flags() & (~B_PULSE_NEEDED));
	Invalidate();
}


bool 
Activity::IsRunning()	
{
	return fIsRunning;
}


void 
Activity::Pulse()
{
	uchar tmp = fPattern.data[7];

	for (int j = 7; j > 0; --j)
		fPattern.data[j] = fPattern.data[j - 1];
	
	fPattern.data[0] = tmp;
	
	Invalidate();
}


void 
Activity::Draw(BRect rect)
{
	BRect viewRect = Bounds();
	BRect bitmapRect = fBitmap->Bounds();

	if (bitmapRect != viewRect) {
		delete fBitmap;
		_CreateBitmap();
	}

	_DrawOnBitmap(IsRunning());
	SetDrawingMode(B_OP_COPY);
	DrawBitmap(fBitmap);
}


void
Activity::_DrawOnBitmap(bool running)
{
	if (fBitmap->Lock())
	{
		BRect rect = fBitmap->Bounds();

		fBitmapView->SetDrawingMode(B_OP_COPY);
	
		rgb_color color;
		color.red = 0;
		color.green = 0;
		color.blue = 0;
		color.alpha = 255;
		
		if (running)
			color.blue = 200;
		
		fBitmapView->SetHighColor(color);

		// draw the pole
		rect.InsetBy(2, 2);
		fBitmapView->FillRect(rect, fPattern);	
		
		// draw frame

		// left
		color.red = 150;
		color.green = 150;
		color.blue = 150;
		fBitmapView->SetHighColor(color);
		fBitmapView->SetDrawingMode(B_OP_OVER);
		BPoint point_a = fBitmap->Bounds().LeftTop();
		BPoint point_b = fBitmap->Bounds().LeftBottom();
		point_b.y -= 1;
		fBitmapView->StrokeLine(point_a, point_b);
		point_a.x += 1;
		point_b.x += 1;
		point_b.y -= 1;
		fBitmapView->StrokeLine(point_a, point_b);

		// top
		point_a = fBitmap->Bounds().LeftTop();
		point_b = fBitmap->Bounds().RightTop();
		point_b.x -= 1;
		fBitmapView->StrokeLine(point_a, point_b);
		point_a.y += 1;
		point_b.y += 1;
		point_b.x -= 1;
		fBitmapView->StrokeLine(point_a, point_b);

		// right
		color.red = 255;
		color.green = 255;
		color.blue = 255;
		fBitmapView->SetHighColor(color);
		point_a = fBitmap->Bounds().RightTop();
		point_b = fBitmap->Bounds().RightBottom();
		fBitmapView->StrokeLine(point_a, point_b);
		point_a.y += 1;
		point_a.x -= 1;
		point_b.x -= 1;
		fBitmapView->StrokeLine(point_a, point_b);

		// bottom
		point_a = fBitmap->Bounds().LeftBottom();
		point_b = fBitmap->Bounds().RightBottom();
		fBitmapView->StrokeLine(point_a, point_b);
		point_a.x += 1;
		point_a.y -= 1;
		point_b.y -= 1;
		fBitmapView->StrokeLine(point_a, point_b);		
		
		// some blending
		color.red = 150;
		color.green = 150;
		color.blue = 150;
		fBitmapView->SetHighColor(color);
		fBitmapView->SetDrawingMode(B_OP_SUBTRACT);
		fBitmapView->StrokeRect(rect);
	
		rect.InsetBy(1, 1);
		_LightenBitmapHighColor(& color);
		fBitmapView->StrokeRect(rect);
		
		rect.InsetBy(1, 1);
		_LightenBitmapHighColor(& color);
		fBitmapView->StrokeRect(rect);
		
		rect.InsetBy(1, 1);
		_LightenBitmapHighColor(& color);
		fBitmapView->StrokeRect(rect);
		
		rect.InsetBy(1, 1);
		_LightenBitmapHighColor(& color);
		fBitmapView->StrokeRect(rect);
		
		fBitmapView->Sync();
		fBitmap->Unlock();
	}
}


void
Activity::_LightenBitmapHighColor(rgb_color* color)
{
	color->red -= 30;
	color->green -= 30;
	color->blue -= 30;
	
	fBitmapView->SetHighColor(*color);
}


void
Activity::_CreateBitmap(void)
{
	BRect rect = Bounds();
	fBitmap = new BBitmap(rect, B_CMAP8, true);
	fBitmapView = new BView(Bounds(), "buffer", B_FOLLOW_NONE, 0);
	fBitmap->AddChild(fBitmapView);
}


void
Activity::FrameResized(float width, float height)
{
	delete fBitmap;
	_CreateBitmap();
	Invalidate();
}

