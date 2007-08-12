/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */

#include "PictureTestCases.h"

void testEmptyPicture(BView *view, BRect frame)
{
	// no op
}

void testDrawString(BView *view, BRect frame)
{
	BFont font;
	view->GetFont(&font);
	font_height height;
	font.GetHeight(&height);
	float baseline = frame.bottom - height.descent;
	// draw base line
	view->SetHighColor(0, 255, 0);
	view->StrokeLine(BPoint(frame.left, baseline - 1), BPoint(frame.right, baseline -1));

	view->SetHighColor(0, 0, 0);
	view->DrawString("Haiku [ÖÜÄöüä]", BPoint(frame.left, baseline));
}

void testFillRed(BView *view, BRect frame)
{
	frame.InsetBy(2, 2);
	view->SetHighColor(255, 0, 0);
	view->FillRect(frame);
}

void testStrokeRect(BView *view, BRect frame)
{
	frame.InsetBy(2, 2);
	int levels = (int)(frame.Height()/2 + 1);
	for (int i = 0; i < levels; i ++) {
		view->SetHighColor(0, 0, 255 * (levels-i) / levels);
		view->StrokeRect(frame);
		frame.InsetBy(1, 1);
	}
}

void testDiagonalLine(BView *view, BRect frame)
{
	view->StrokeLine(BPoint(frame.left, frame.top), BPoint(frame.right, frame.bottom));
}

void testStrokeScaledRect(BView *view, BRect frame)
{
	view->SetScale(0.5);
	view->StrokeRect(frame);
}

void testRecordPicture(BView *view, BRect frame)
{
	BPicture *picture = new BPicture();
	view->BeginPicture(picture);
	view->FillRect(frame);
	view->EndPicture();
	delete picture;
}

void testRecordAndPlayPicture(BView *view, BRect frame)
{
	BPicture *picture = new BPicture();
	view->BeginPicture(picture);
	frame.InsetBy(2, 2);
	view->FillRect(frame);
	view->EndPicture();
	view->DrawPicture(picture);
	delete picture;
}

void testRecordAndPlayPictureWithOffset(BView *view, BRect frame)
{
	BPicture *picture = new BPicture();
	view->BeginPicture(picture);
	frame.InsetBy(frame.Width() / 4, frame.Height() / 4);
	frame.OffsetTo(0, 0);
	view->FillRect(frame);
	view->EndPicture();
	
	view->DrawPicture(picture, BPoint(10, 10));
	// color of picture should not change
	view->SetLowColor(0, 255, 0);
	view->SetLowColor(255, 0, 0);
	view->DrawPicture(picture, BPoint(0, 0));
	delete picture;
}

void testBitmap(BView *view, BRect frame) {
	BBitmap bitmap(frame, B_RGBA32);
	for (int32 y = 0; y < bitmap.Bounds().IntegerHeight(); y ++) {
		for (int32 x = 0; x < bitmap.Bounds().IntegerWidth(); x ++) {
			char *pixel = (char*)bitmap.Bits();
			pixel += bitmap.BytesPerRow() * y + 4 * x;
			// fill with blue
			pixel[0] = 255;
			pixel[1] = 0;
			pixel[2] = 0;
			pixel[3] = 255;
		}
	}
	view->DrawBitmap(&bitmap, BPoint(0, 0));
}

TestCase gTestCases[] = {
	{ "Test Empty Picture", testEmptyPicture },
	{ "Test Diagonal Line", testDiagonalLine },
	{ "Test Stroke Rect", testStrokeRect },
	{ "Test Draw String", testDrawString },
	{ "Test Fill Red", testFillRed },
	{ "Test Stroke Scaled Rect", testStrokeScaledRect },
	{ "Test Record Picture", testRecordPicture },
	{ "Test Record And Play Picture", testRecordAndPlayPicture },
	{ "Test Record And Play Picture With Offset", testRecordAndPlayPictureWithOffset },
	{ "Test Draw Bitmap", testBitmap },
	{ NULL, NULL }
};