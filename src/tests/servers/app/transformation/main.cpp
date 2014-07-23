/*
 * Copyright 2014 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include <algorithm>
#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Bitmap.h>
#include <GradientLinear.h>
#include <Picture.h>
#include <Region.h>
#include <Resources.h>
#include <Roster.h>
#include <String.h>
#include <StringView.h>

#include "harness.h"

static const char* kAppSignature = "application/x-vnd.Haiku-Transformation";


class BitmapTest : public Test {
public:
	BitmapTest(const char* name)
		:
		Test(name),
		fBitmap(_LoadBitmap(555))
	{
	}

private:
	status_t
	_GetAppResources(BResources& resources) const
	{
		app_info info;
		status_t status = be_app->GetAppInfo(&info);
		if (status != B_OK)
			return status;
	
		return resources.SetTo(&info.ref);
	}


	BBitmap* _LoadBitmap(int resourceID) const
	{
		BResources resources;
		status_t status = _GetAppResources(resources);
		if (status != B_OK)
			return NULL;
	
		size_t dataSize;
		const void* data = resources.LoadResource(B_MESSAGE_TYPE, resourceID,
			&dataSize);
		if (data == NULL)
			return NULL;

		BMemoryIO stream(data, dataSize);

		// Try to read as an archived bitmap.
		BMessage archive;
		status = archive.Unflatten(&stream);
		if (status != B_OK)
			return NULL;

		BBitmap* bitmap = new BBitmap(&archive);

		status = bitmap->InitCheck();
		if (status != B_OK) {
			delete bitmap;
			bitmap = NULL;
		}

		return bitmap;
	}

protected:
	BBitmap*	fBitmap;
};


// #pragma mark - Test1


class RectsTest : public Test {
public:
	RectsTest()
		:
		Test("Rects")
	{
	}
	
	virtual void Draw(BView* view, BRect updateRect)
	{
		view->DrawString("Rects", BPoint(20, 30));

		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

		BRect rect(view->Bounds());
		rect.OffsetTo(B_ORIGIN);

		rect.InsetBy(rect.Width() / 3, rect.Height() / 3);
		BPoint center(
			rect.left + rect.Width() / 2,
			rect.top + rect.Height() / 2);

		for (int32 i = 0; i < 360; i += 40) {
			BAffineTransform transform;
			transform.RotateBy(center, i * M_PI / 180.0);
			view->SetTransform(transform);

			view->SetHighColor(51, 151, 255, 20);
			view->FillRect(rect);

			view->SetHighColor(51, 255, 151, 180);
			view->DrawString("Rect", center);
		}
	}
};


// #pragma mark - AlphaMaskBitmapTest


class AlphaMaskBitmapTest : public BitmapTest {
public:
	AlphaMaskBitmapTest()
		:
		BitmapTest("Alpha Masked Bitmap")
	{
	}

	virtual void Draw(BView* view, BRect updateRect)
	{
		BRect rect(view->Bounds());

		if (fBitmap == NULL) {
			view->SetHighColor(255, 0, 0);
			view->FillRect(rect);
			view->SetHighColor(0, 0, 0);
			view->DrawString("Failed to load the bitmap.", BPoint(20, 20));
			return;
		}

		rect.left = (rect.Width() - fBitmap->Bounds().Width()) / 2;
		rect.top = (rect.Height() - fBitmap->Bounds().Height()) / 2;
		rect.right = rect.left + fBitmap->Bounds().Width();
		rect.bottom = rect.top + fBitmap->Bounds().Height();

		BPoint center(
			rect.left + rect.Width() / 2,
			rect.top + rect.Height() / 2);

		BPicture picture;
		view->BeginPicture(&picture);
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
		BFont font;
		view->GetFont(&font);
		font.SetSize(70);
		view->SetFont(&font);
		view->SetHighColor(0, 0, 0, 80);
		view->FillRect(view->Bounds());
		view->SetHighColor(0, 0, 0, 255);
		view->DrawString("CLIPPING", BPoint(0, center.y + 35));
		view->EndPicture();

		view->ClipToPicture(&picture);

		BAffineTransform transform;
			transform.RotateBy(center, 30 * M_PI / 180.0);
			view->SetTransform(transform);
	
		view->DrawBitmap(fBitmap, fBitmap->Bounds(), rect);
	}
};


// #pragma mark - Gradient


class GradientTest : public Test {
public:
	GradientTest()
		:
		Test("Gradient")
	{
	}
	
	virtual void Draw(BView* view, BRect updateRect)
	{
		BRect rect(view->Bounds());
		rect.InsetBy(rect.Width() / 3, rect.Height() / 3);
		BPoint center(
			rect.left + rect.Width() / 2,
			rect.top + rect.Height() / 2);

		BAffineTransform transform;
		transform.RotateBy(center, 30.0 * M_PI / 180.0);
		view->SetTransform(transform);

		rgb_color top = (rgb_color){ 255, 255, 0, 255 };
		rgb_color bottom = (rgb_color){ 0, 255, 255, 255 };

		BGradientLinear gradient;
		gradient.AddColor(top, 0.0f);
		gradient.AddColor(bottom, 255.0f);
		gradient.SetStart(rect.LeftTop());
		gradient.SetEnd(rect.LeftBottom());

		float radius = std::min(rect.Width() / 5, rect.Height() / 5);

		view->FillRoundRect(rect, radius, radius, gradient);
	}
};


// #pragma mark - NestedStates


class NestedStatesTest : public Test {
public:
	NestedStatesTest()
		:
		Test("Nested view states")
	{
	}
	
	virtual void Draw(BView* view, BRect updateRect)
	{
		BAffineTransform transform;
		transform.RotateBy(BPoint(100, 100), 30.0 * M_PI / 180.0);
		view->SetTransform(transform);

		rgb_color top = (rgb_color){ 255, 0, 0, 255 };
		rgb_color bottom = (rgb_color){ 255, 255, 0, 255 };

		BRect rect(20, 20, 120, 120);

		BGradientLinear gradient;
		gradient.AddColor(top, 0.0f);
		gradient.AddColor(bottom, 255.0f);
		gradient.SetStart(rect.LeftTop());
		gradient.SetEnd(rect.LeftBottom());

		view->FillRoundRect(rect, 20, 20, gradient);

		view->PushState();
		// Should be in the same place!
		view->StrokeRoundRect(rect, 20, 20);

		// Now rotated by another 30 degree
		view->SetTransform(transform);

		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
		view->SetHighColor(0, 0, 255, 120);
		view->FillRoundRect(rect, 20, 20);

		view->PopState();
	}
};


// #pragma mark - Clipping


class ClippingTest : public Test {
public:
	ClippingTest()
		:
		Test("View bounds clipping")
	{
	}

	virtual void Draw(BView* view, BRect updateRect)
	{
		BRect r (20, 20, 50, 50);
		view->SetHighColor(ui_color(B_FAILURE_COLOR));
		view->FillRect(r);

		BAffineTransform transform;
		transform.TranslateBy(400, 400);
		view->SetTransform(transform);

		// Make sure this rectangle is drawn, even when the original one is out
		// of the view bounds (for example because of scrolling).
		view->SetHighColor(ui_color(B_SUCCESS_COLOR));
		view->FillRect(r);
	}
};


// #pragma mark - Clipping


class TextClippingTest : public Test {
public:
	TextClippingTest()
		:
		Test("Text clipping")
	{
	}

	virtual void Draw(BView* view, BRect updateRect)
	{
		BFont font;
		view->GetFont(&font);
		font.SetSize(70);
		view->SetFont(&font);

		float width = view->Bounds().Width();

		// The translation make the text, which has negative coordinates, be
		// visible inside the viewport.
		BAffineTransform transform;
		transform.TranslateBy(width, 0);
		view->SetTransform(transform);

		const char* str = "CLIPPING";

		// Test the standard DrawString method

		// Draw the text bounds
		float size = view->StringWidth(str);
		BRect r(-width, 0, size - width, 70);
		view->SetHighColor(ui_color(B_SUCCESS_COLOR));
		view->FillRect(r);

		// Draw the text (which should fit inside the bounds rectangle)
		view->SetHighColor(0, 0, 0, 255);
		view->DrawString(str, BPoint(-width, 70));

		// Test with offset-based DrawString
		BPoint offsets[strlen(str)];
		for(unsigned int i = 0; i < strlen(str); i++)
		{
			offsets[i].x = i * 35 - width;
			offsets[i].y = 145;
		}

		// Draw the text bounds
		view->SetHighColor(ui_color(B_SUCCESS_COLOR));
		r = BRect(offsets[0], offsets[strlen(str) - 1]);
		r.top = 75;
		view->FillRect(r);

		// Draw the text (which should fit inside the bounds rectangle)
		view->SetHighColor(0, 0, 0, 255);
		view->DrawString(str, offsets, strlen(str));

	}
};


// #pragma mark - BitmapClipTest


class BitmapClipTest : public BitmapTest {
public:
	BitmapClipTest()
		:
		BitmapTest("Bitmap clipping")
	{
	}

	virtual void Draw(BView* view, BRect updateRect)
	{
		BRect rect(view->Bounds());

		if (fBitmap == NULL) {
			view->SetHighColor(255, 0, 0);
			view->FillRect(rect);
			view->SetHighColor(0, 0, 0);
			view->DrawString("Failed to load the bitmap.", BPoint(20, 20));
			return;
		}

		rect = fBitmap->Bounds();

		view->SetHighColor(ui_color(B_FAILURE_COLOR));
		view->FillRect(rect);

		// The rect offset should compensate the transform translation, so the
		// bitmap should be drawn at the view origin. It will then exactly
		// cover the red rectangle, which should not be visible anymore.
		rect.OffsetBy(0, 40);

		BAffineTransform transform;
			transform.TranslateBy(0, -40);
		view->SetTransform(transform);

		view->DrawBitmap(fBitmap, fBitmap->Bounds(), rect);
	}
};


// #pragma mark - PixelAlignTest


class PixelAlignTest : public Test {
public:
	PixelAlignTest()
		:
		Test("Pixel alignment")
	{
	}

	virtual void Draw(BView* view, BRect updateRect)
	{
		BRect rect(20, 20, 120, 120);
		view->SetHighColor(ui_color(B_SUCCESS_COLOR));
		view->StrokeRect(rect);

		BAffineTransform transform;
			transform.TranslateBy(140, 0);
		view->SetTransform(transform);

		// Translating a pixel-aligned rectangle by an integer number of
		// pixels should result in a pixel-aligned rectangle.
		view->SetHighColor(ui_color(B_FAILURE_COLOR));
		view->StrokeRect(rect);
	}
};


// #pragma mark -


int
main(int argc, char** argv)
{
	BApplication app(kAppSignature);

	TestWindow* window = new TestWindow("Transformation tests");

	window->AddTest(new RectsTest());
	window->AddTest(new BitmapClipTest());
	window->AddTest(new TextClippingTest());
	window->AddTest(new AlphaMaskBitmapTest());
	window->AddTest(new GradientTest());
	window->AddTest(new NestedStatesTest());
	window->AddTest(new ClippingTest());
	window->AddTest(new PixelAlignTest());

	window->SetToTest(2);
	window->Show();

	app.Run();
	return 0;
}
