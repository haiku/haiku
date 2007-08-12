/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */

#include "PictureTestCases.h"

static const rgb_color kBlack = {0, 0, 0};
static const rgb_color kWhite = {255, 255, 255};
static const rgb_color kRed = {255, 0, 0};
static const rgb_color kGreen = {0, 255, 0};
static const rgb_color kBlue = {0, 0, 255};

static BPoint centerPoint(BRect rect)
{
	int x = (int)(rect.left + rect.IntegerWidth() / 2);
	int y = (int)(rect.top + rect.IntegerHeight() / 2);
	return BPoint(x, y);
}

static void testNoOp(BView *view, BRect frame)
{
	// no op
}

static void testDrawChar(BView *view, BRect frame)
{
	view->MovePenTo(frame.left, frame.bottom - 5);
	view->DrawChar('A');
	
	view->DrawChar('B', BPoint(frame.left + 20, frame.bottom - 5));
}

static void testDrawString(BView *view, BRect frame)
{
	BFont font;
	view->GetFont(&font);
	font_height height;
	font.GetHeight(&height);
	float baseline = frame.bottom - height.descent;
	// draw base line
	view->SetHighColor(kGreen);
	view->StrokeLine(BPoint(frame.left, baseline - 1), BPoint(frame.right, baseline -1));

	view->SetHighColor(kBlack);
	view->DrawString("Haiku [ÖÜÄöüä]", BPoint(frame.left, baseline));
}

static void testDrawStringWithLength(BView *view, BRect frame)
{
	BFont font;
	view->GetFont(&font);
	font_height height;
	font.GetHeight(&height);
	float baseline = frame.bottom - height.descent;
	// draw base line
	view->SetHighColor(kGreen);
	view->StrokeLine(BPoint(frame.left, baseline - 1), BPoint(frame.right, baseline -1));

	view->SetHighColor(kBlack);
	view->DrawString("Haiku [ÖÜÄöüä]", 13, BPoint(frame.left, baseline));
}

static void testFillArc(BView *view, BRect frame)
{
	frame.InsetBy(2, 2);
	view->FillArc(frame, 45, 180);
}

static void testStrokeArc(BView *view, BRect frame)
{
	frame.InsetBy(2, 2);
	view->StrokeArc(frame, 45, 180);
}

static void testFillBezier(BView *view, BRect frame)
{
	frame.InsetBy(2, 2);
	BPoint points[4];
	points[0] = BPoint(frame.left, frame.bottom);
	points[1] = BPoint(frame.left, frame.top);
	points[1] = BPoint(frame.left, frame.top);
	points[3] = BPoint(frame.right, frame.top);
	view->FillBezier(points);
}

static void testStrokeBezier(BView *view, BRect frame)
{
	frame.InsetBy(2, 2);
	BPoint points[4];
	points[0] = BPoint(frame.left, frame.bottom);
	points[1] = BPoint(frame.left, frame.top);
	points[1] = BPoint(frame.left, frame.top);
	points[3] = BPoint(frame.right, frame.top);
	view->StrokeBezier(points);
}

static void testFillEllipse(BView *view, BRect frame)
{
	frame.InsetBy(2, 2);
	view->FillEllipse(frame);

	view->SetHighColor(kRed);
	float r = frame.Width() / 3;
	float s = frame.Height() / 4;
	view->FillEllipse(centerPoint(frame), r, s);
}

static void testStrokeEllipse(BView *view, BRect frame)
{
	frame.InsetBy(2, 2);
	view->StrokeEllipse(frame);

	view->SetHighColor(kRed);
	float r = frame.Width() / 3;
	float s = frame.Height() / 4;
	view->StrokeEllipse(centerPoint(frame), r, s);
}

static void testFillPolygon(BView *view, BRect frame)
{
	frame.InsetBy(2, 2);

	BPoint points[4];
	points[0] = BPoint(frame.left, frame.top);
	points[1] = BPoint(frame.right, frame.bottom);
	points[2] = BPoint(frame.right, frame.top);
	points[3] = BPoint(frame.left, frame.bottom);

	view->FillPolygon(points, 4);
}

static void testStrokePolygon(BView *view, BRect frame)
{
	frame.InsetBy(2, 2);

	BPoint points[4];
	points[0] = BPoint(frame.left, frame.top);
	points[1] = BPoint(frame.right, frame.bottom);
	points[2] = BPoint(frame.right, frame.top);
	points[3] = BPoint(frame.left, frame.bottom);

	view->StrokePolygon(points, 4);
}

static void testFillRect(BView *view, BRect frame)
{
	frame.InsetBy(2, 2);
	view->FillRect(frame);
}

static void testStrokeRect(BView *view, BRect frame)
{
	frame.InsetBy(2, 2);
	view->StrokeRect(frame);
}

static void testFillRegion(BView *view, BRect frame)
{
	frame.InsetBy(2, 2);
	BRegion region(frame);
	frame.InsetBy(2, 2);
	region.Exclude(frame);
	view->FillRegion(&region);
}

static void testFillRoundRect(BView *view, BRect frame)
{
	frame.InsetBy(2, 2);
	view->FillRoundRect(frame, 5, 3);
}

static void testStrokeRoundRect(BView *view, BRect frame)
{
	frame.InsetBy(2, 2);
	view->StrokeRoundRect(frame, 5, 3);
}

static void testFillTriangle(BView *view, BRect frame)
{
	frame.InsetBy(2, 2);
	BPoint points[3];
	points[0] = BPoint(frame.left, frame.bottom);
	points[1] = BPoint(centerPoint(frame).x, frame.top);
	points[2] = BPoint(frame.right, frame.bottom);
	view->FillTriangle(points[0], points[1], points[2]); 
}

static void testStrokeTriangle(BView *view, BRect frame)
{
	frame.InsetBy(2, 2);
	BPoint points[3];
	points[0] = BPoint(frame.left, frame.bottom);
	points[1] = BPoint(centerPoint(frame).x, frame.top);
	points[2] = BPoint(frame.right, frame.bottom);
	view->StrokeTriangle(points[0], points[1], points[2]); 
}

static void testStrokeLine(BView *view, BRect frame)
{
	frame.InsetBy(2, 2);
	view->StrokeLine(BPoint(frame.left, frame.top), BPoint(frame.right, frame.top));
	
	frame.top += 2;
	frame.bottom -= 2;
	view->StrokeLine(BPoint(frame.left, frame.top), BPoint(frame.right, frame.bottom));
	
	frame.bottom += 2;;
	frame.top = frame.bottom;
	view->StrokeLine(BPoint(frame.right, frame.top), BPoint(frame.left, frame.top));
}

static void testFillShape(BView *view, BRect frame)
{
	frame.InsetBy(2, 2);
	BShape shape;
	shape.MoveTo(BPoint(frame.left, frame.bottom));
	shape.LineTo(BPoint(frame.right, frame.top));
	shape.LineTo(BPoint(frame.left, frame.top));
	shape.LineTo(BPoint(frame.right, frame.bottom));
	view->FillShape(&shape);
}

static void testStrokeShape(BView *view, BRect frame)
{
	frame.InsetBy(2, 2);
	BShape shape;
	shape.MoveTo(BPoint(frame.left, frame.bottom));
	shape.LineTo(BPoint(frame.right, frame.top));
	shape.LineTo(BPoint(frame.left, frame.top));
	shape.LineTo(BPoint(frame.right, frame.bottom));
	view->StrokeShape(&shape);
}

static void testRecordPicture(BView *view, BRect frame)
{
	BPicture *picture = new BPicture();
	view->BeginPicture(picture);
	view->FillRect(frame);
	view->EndPicture();
	delete picture;
}

static void testRecordAndPlayPicture(BView *view, BRect frame)
{
	BPicture *picture = new BPicture();
	view->BeginPicture(picture);
	frame.InsetBy(2, 2);
	view->FillRect(frame);
	view->EndPicture();
	view->DrawPicture(picture);
	delete picture;
}

static void testRecordAndPlayPictureWithOffset(BView *view, BRect frame)
{
	BPicture *picture = new BPicture();
	view->BeginPicture(picture);
	frame.InsetBy(frame.Width() / 4, frame.Height() / 4);
	frame.OffsetTo(0, 0);
	view->FillRect(frame);
	view->EndPicture();
	
	view->DrawPicture(picture, BPoint(10, 10));
	// color of picture should not change
	view->SetLowColor(kGreen);
	view->SetLowColor(kRed);
	view->DrawPicture(picture, BPoint(0, 0));
	delete picture;
}

static void testAppendToPicture(BView *view, BRect frame)
{
	frame.InsetBy(2, 2);
	view->BeginPicture(new BPicture());
	view->FillRect(frame);
	BPicture* picture = view->EndPicture();
	if (picture == NULL)
		return;
	
	frame.InsetBy(2, 2);
	view->AppendToPicture(picture);
	view->SetHighColor(kRed);
	view->FillRect(frame);
	if (view->EndPicture() != picture)
		return;
	
	view->DrawPicture(picture);
	delete picture;
}

static void testLineArray(BView *view, BRect frame)
{
	frame.InsetBy(2, 2);
	view->BeginLineArray(3);
	view->AddLine(BPoint(frame.left, frame.top), BPoint(frame.right, frame.top), kBlack);
	
	frame.top += 2;
	frame.bottom -= 2;
	view->AddLine(BPoint(frame.left, frame.top), BPoint(frame.right, frame.bottom), kRed);
	
	frame.bottom += 2;;
	frame.top = frame.bottom;
	view->AddLine(BPoint(frame.right, frame.top), BPoint(frame.left, frame.top), kGreen);	
	
	view->EndLineArray();
}

static void testCopyBits(BView *view, BRect frame)
{
	frame.InsetBy(2, 2);
	BRect leftHalf(frame);
	leftHalf.right = centerPoint(frame).x - 1;
	
	BRect rightHalf(frame);
	rightHalf.left = centerPoint(frame).x + 1;
	
	view->StrokeRect(leftHalf);
	
	view->CopyBits(leftHalf, rightHalf);
}

static void testInvertRect(BView *view, BRect frame)
{
	frame.InsetBy(2, 2);
	view->InvertRect(frame);
}

static void testBitmap(BView *view, BRect frame) {
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
	{ "Test No Operation", testNoOp },
	{ "Test DrawChar", testDrawChar },
	{ "Test Draw String", testDrawString },
	{ "Test Draw String With Length", testDrawStringWithLength },
	{ "Test FillArc", testFillArc },
	{ "Test StrokeArc", testStrokeArc },
	{ "Test FillBezier", testFillBezier },
	{ "Test StrokeBezier", testStrokeBezier },
	{ "Test FillEllipse", testFillEllipse },
	{ "Test StrokeEllipse", testStrokeEllipse },
	{ "Test FillPolygon", testFillPolygon },
	{ "Test StrokePolygon", testStrokePolygon },
	{ "Test FillRect", testFillRect },
	{ "Test StrokeRect", testStrokeRect },
	{ "Test FillRegion", testFillRegion },
	{ "Test FillRoundRect", testFillRoundRect },
	{ "Test StrokeRoundRect", testStrokeRoundRect },
	{ "Test FillTriangle", testFillTriangle },
	{ "Test StrokeTriangle", testStrokeTriangle },
	{ "Test StrokeLine", testStrokeLine },
	{ "Test FillShape", testFillShape },
	{ "Test StrokeShape", testStrokeShape },
	{ "Test Record Picture", testRecordPicture },
	{ "Test Record And Play Picture", testRecordAndPlayPicture },
	{ "Test Record And Play Picture With Offset", testRecordAndPlayPictureWithOffset },
	{ "Test AppendToPicture", testAppendToPicture },
	{ "Test LineArray", testLineArray },
	// does not work under R5 for pictures!
	{ "Test CopyBits*", testCopyBits },
	{ "Test InvertRect", testInvertRect },
	{ "Test Bitmap", testBitmap },
	{ NULL, NULL }
};

