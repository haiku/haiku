// main.cpp

#include <stdio.h>

#include <Application.h>
#include <Bitmap.h>
#include <GraphicsDefs.h>
#include <Region.h>
#include <Window.h>

#include "BitmapView.h"
#include "BitmapBuffer.h"
#include "FontManager.h"
#include "Painter.h"

const pattern kStripes = (pattern){ { 0xc7, 0x8f, 0x1f, 0x3e, 0x7c, 0xf8, 0xf1, 0xe3 } };
const pattern kDotted = (pattern){ { 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa } };
const pattern kDottedBigger = (pattern){ { 0x33, 0x33, 0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc } };

// test_straight_lines
template<class Surface>
bigtime_t
test_straight_lines(Surface& s, uint32 width, uint32 height)
{
	bigtime_t now = system_time();

	s.SetPenSize(1.0);
	s.SetDrawingMode(B_OP_COPY);
	s.SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

	s.SetHighColor(0, 0, 0, 255);
	s.SetLowColor(130, 0, 20, 255);

	const pattern pat = B_SOLID_HIGH;

	for (uint32 y = 0; y <= height; y += 5) {
		s.StrokeLine(BPoint(0, y), BPoint(width - 1, y), pat);
	}
	for (uint32 x = 0; x <= width; x += 5) {
		s.StrokeLine(BPoint(x, 0), BPoint(x, height - 1), pat);
	}

	s.Sync();

	return system_time() - now;	
}

// test_fill_rect
template<class Surface>
bigtime_t
test_fill_rect(Surface& s, uint32 width, uint32 height)
{
	bigtime_t now = system_time();

	s.SetPenSize(1.0);
	s.SetDrawingMode(B_OP_COPY);
	s.SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

	s.SetHighColor(0, 0, 0, 255);
	s.SetLowColor(130, 0, 20, 255);

	const pattern pat = B_SOLID_HIGH;

	BRect r;
	for (uint32 y = 10; y <= height; y += 20) {
		for (uint32 x = 10; x <= width; x += 20) {
			r.Set(x - 9, y - 9, x + 9, y + 9);
			s.FillRect(r, pat);
		}
	}

	s.Sync();

	return system_time() - now;	
}

// test_ellipses
template<class Surface>
bigtime_t
test_ellipses(Surface& s, uint32 width, uint32 height)
{
	bigtime_t now = system_time();

	s.SetPenSize(2.0);
	s.SetDrawingMode(B_OP_COPY);
	s.SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

	s.SetHighColor(0, 0, 0, 255);
	s.SetLowColor(130, 0, 20, 255);

	const pattern pat = B_SOLID_HIGH;

	BPoint center(floorf(width / 2.0), floorf(height / 2.0));
	float xRadius = width / 2.0;
	float yRadius = height / 2.0;

	uint32 count = 40;
	for (uint32 i = 0; i < count; i ++) {
		s.StrokeEllipse(center, xRadius * (i / (float)count),
								yRadius * (i / (float)count), pat);
	}

	s.Sync();

	return system_time() - now;	
}

// test_lines
template<class Surface>
bigtime_t
test_lines(Surface& s, uint32 width, uint32 height)
{
	bigtime_t now = system_time();

	s.SetPenSize(1.0);
	s.SetDrawingMode(B_OP_COPY);
	s.SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

	s.SetHighColor(0, 0, 0, 255);
	s.SetLowColor(130, 0, 20, 255);

	const pattern pat = B_SOLID_HIGH;

	for (uint32 y = 0; y <= height; y += 10) {
		s.StrokeLine(BPoint(0, 0), BPoint(width, y), pat);
		s.StrokeLine(BPoint(width - 1, 0), BPoint(0, y), pat);
	}
	for (uint32 x = 0; x <= width; x += 10) {
		s.StrokeLine(BPoint(0, 0), BPoint(x, height), pat);
		s.StrokeLine(BPoint(width - 1, 0), BPoint(x, height), pat);
	}

	s.Sync();

	return system_time() - now;	
}

// test
template<class Surface>
bigtime_t
test(Surface& s, uint32 width, uint32 height, BBitmap* testBitmap)
{
	bigtime_t now = system_time();

// TODO: Painter behaves differently when origin has subpixel offset
//	BPoint origin(20.3, 10.8);
	BPoint origin(20, 10);
	
	BPoint center(width / 2.0, height / 2.0);
	float xRadius = 30.0;
	float yRadius = 20.0;

	s.SetOrigin(origin);
	s.SetScale(1.0);

	s.SetDrawingMode(B_OP_COPY);
//	s.SetDrawingMode(B_OP_SUBTRACT);
//	s.SetDrawingMode(B_OP_OVER);
	s.SetHighColor(20, 20, 20, 255);
	s.SetLowColor(220, 120, 80, 255);
	for (uint32 y = 0; y <= height / 2; y += 10) 
		s.StrokeLine(BPoint(0, 0), BPoint(width / 2, y)/*, kDottedBigger*/);
	for (uint32 x = 0; x <= width; x += 10)
		s.StrokeLine(BPoint(0, 0), BPoint(x, height)/*, kDottedBigger*/);
	s.SetPenSize(1.0 * 5);
	s.SetHighColor(255, 0, 0, 255);
	s.SetLowColor(0, 0, 255, 255);
//	s.SetScale(1.0);
//	s.SetOrigin(B_ORIGIN);
//	s.SetDrawingMode(B_OP_INVERT);
	s.SetDrawingMode(B_OP_COPY);
//	s.StrokeRect(BRect(20.2, 45.6, 219.0, 139.0));
	s.StrokeRect(BRect(20.2, 45.6, 219.0, 139.0), kStripes);

//	s.ConstrainClipping(noClip);
/*	s.SetPenLocation(BPoint(230.0, 30.0));
	s.StrokeLine(BPoint(250.0, 30.0));
	s.StrokeLine(BPoint(250.0, 50.0));
	s.StrokeLine(BPoint(230.0, 50.0));
	s.StrokeLine(BPoint(230.0, 30.0));*/

	s.SetHighColor(255, 255, 0, 255);
	s.SetLowColor(128, 0, 50, 255);
//	s.SetDrawingMode(B_OP_OVER);
	s.SetDrawingMode(B_OP_ERASE);
	s.StrokeEllipse(center, xRadius, yRadius, kDottedBigger);
	s.SetHighColor(255, 0, 0, 255);
	s.SetDrawingMode(B_OP_INVERT);
	s.FillArc(center, xRadius * 2, yRadius * 2, 40.0, 230.0);
//	s.StrokeArc(center, xRadius * 2, yRadius * 2, 40.0, 230.0);
	s.SetDrawingMode(B_OP_OVER);
	s.SetPenSize(2.0);
	s.StrokeEllipse(center, xRadius * 3, yRadius * 2);
//	s.FillEllipse(center, xRadius * 3, yRadius * 2);
//	s.StrokeLine(bounds.RightTop());
//	s.FillRect(rect);
	s.SetHighColor(0, 0, 255, 255);
	s.SetLowColor(255, 0, 0, 255);
	s.SetPenSize(1.0);
//	s.SetDrawingMode(B_OP_SELECT);
	s.StrokeRoundRect(BRect(40, 100, 250, 220.0), 40, 40);
//	s.FillRoundRect(BRect(40, 100, 250, 220.0), 40, 40);

	// text rendering
	const char* string1 = "The Quick Brown Fox...";
	const char* string2 = "jumps!";
	BPoint stringLocation1(10.0, 220.0);
	BPoint stringLocation2(30.0 / 2.5, 115.0 / 2.5);

	BFont font(be_plain_font);
	font.SetSize(12.0);
	font.SetRotation(8.0);
//	font.SetFamilyAndStyle(1);

	s.SetFont(&font);
	s.SetHighColor(91, 105, 98, 120);
	s.SetDrawingMode(B_OP_OVER);
	s.DrawString(string1, stringLocation1);
	s.DrawString(string2);
	s.StrokeLine(BPoint(width - 1, 0));

//	s.SetScale(2.5);
	s.SetDrawingMode(B_OP_INVERT);
	s.SetHighColor(200, 200, 200, 255);
	s.DrawString("H", stringLocation2);
	s.DrawString("e");
	s.DrawString("l");
	s.DrawString("l");
	s.DrawString("o");
	s.DrawString(" ");
	s.DrawString("N");
	s.DrawString("u");
	s.DrawString("r");
	s.DrawString("s");
	s.DrawString("e");
	s.DrawString("!");
	// do the char locations match up?
//	s.SetHighColor(0, 60, 240);
//	s.DrawString("Hello Nurse!", stringLocation2);

	// bitmap drawing
	BRect testBitmapCrop(testBitmap->Bounds());
	testBitmapCrop.left += 20.0;
	BRect testBitmapDestRect(testBitmapCrop);
	testBitmapDestRect.OffsetBy(50, 20);

	s.SetScale(1.5);
	s.SetDrawingMode(B_OP_ALPHA);
	s.SetHighColor(0, 0, 0, 120);
	s.DrawBitmap(testBitmap, testBitmapCrop, testBitmapDestRect);

	s.Sync();

	return system_time() - now;
}



// main
int
main(int argc, char **argv)
{
	BApplication* app = new BApplication("application/x.vnd-YellowBites.TestApp");

	// create the default instance of the FontManager
	// It takes a bit to scan through the font files;
	// "true" means to do the font scanning inline, not
	// in a separate thread.
	fprintf(stdout, "scanning font files...");
	fflush(stdout);
	FontManager::CreateDefault(true);
	fprintf(stdout, "done\n");

	BRect bounds(0.0, 0.0, 319.0, 239.0);

	// test the clipping
	BRegion noClip(bounds);

	int32 clipCount = 5;
	BRegion clip;
	float h = bounds.Width() / clipCount;
	float v = bounds.Height() / clipCount;
	float hInset = (h / 2.0) * 0.5;
	float vInset = (v / 2.0) * 0.5;
	for (int32 i = 0; i < clipCount; i++) {
/*		BRect b(h * i, bounds.top, h * i, bounds.bottom);
		b.InsetBy(-hInset, 0.0);
		clip.Include(b);
		b.Set(bounds.left, v * i, bounds.right, v * i);
		b.InsetBy(0.0, -vInset);
		clip.Include(b);*/
		BRect b(bounds.left, v * i, bounds.right, v * i);
		b.InsetBy(0.0, -vInset);
		clip.Include(b);
		b.Set(h * i, bounds.top, h * i, bounds.bottom);
		b.InsetBy(-hInset, 0.0);
		clip.Include(b);
	}

	// prepare a test bitmap for bitmap rendering
	BBitmap* testBitmap = new BBitmap(BRect(20, 0, 150, 50), 0, B_RGB32);
	// fill testBitmap with content
	uint8* bits = (uint8*)testBitmap->Bits();
	uint32 bitmapWidth = testBitmap->Bounds().IntegerWidth() + 1;
	uint32 bitmapHeight = testBitmap->Bounds().IntegerHeight() + 1;
	uint32 bpr = testBitmap->BytesPerRow();
	for (uint32 y = 0; y < bitmapHeight; y++) {
		uint8* h = bits;
		for (uint32 x = 0; x < bitmapWidth; x++) {
			h[0] = (uint8)((float)x / (float)bitmapWidth * 255.0);
			h[1] = (uint8)((float)y / (float)bitmapHeight * 255.0);
			h[2] = 255 - (uint8)((float)y / (float)bitmapHeight * 255.0);
			h[3] = (uint8)((float)y / (float)bitmapHeight * 255.0);
			h += 4;
		}
		bits += bpr;
	}
	// make corners a black pixel for testing
	bits = (uint8*)testBitmap->Bits();
	*(uint32*)(&bits[0]) = 0;
	*(uint32*)(&bits[(bitmapWidth - 1) * 4]) = 0;
	*(uint32*)(&bits[(bitmapHeight - 1) * bpr]) = 0;
	*(uint32*)(&bits[(bitmapHeight - 1) * bpr + (bitmapWidth - 1) * 4]) = 0;

	// create a frame buffer
	BBitmap* bitmap = new BBitmap(bounds, B_RGB32);
//memset(bitmap->Bits(), 0, bitmap->BitsLength());
	BitmapBuffer* buffer = new BitmapBuffer(bitmap);
	Painter painter;
	painter.AttachToBuffer(buffer);

	uint32 width = buffer->Width();
	uint32 height = buffer->Height();

//	painter.ConstrainClipping(clip);

	int32 iterations = 40;

	fprintf(stdout, "Painter...");
	fflush(stdout);

	bigtime_t painterNow = 0;
	for (int32 i = 0; i < iterations; i++) {
		// reset bitmap contents
		memset(bitmap->Bits(), 255, bitmap->BitsLength());
		// run test
//		painterNow += test(painter, width, height, testBitmap);
//		painterNow += test_lines(painter, width, height);
//		painterNow += test_straight_lines(painter, width, height);
		painterNow += test_fill_rect(painter, width, height);
//		painterNow += test_ellipses(painter, width, height);
	}

fprintf(stdout, " %lld µsecs\n", painterNow / iterations);

	BitmapView* painterView = new BitmapView(bounds, "view", bitmap);

	bitmap = new BBitmap(bounds, B_RGB32, true);
	BView* view = new BView(bounds, NULL, B_FOLLOW_NONE, B_WILL_DRAW);
//memset(bitmap->Bits(), 0, bitmap->BitsLength());
	bitmap->Lock();
	bitmap->AddChild(view);

//	view->ConstrainClippingRegion(&clip);

	fprintf(stdout, "BView...");
	fflush(stdout);

	bigtime_t viewNow = 0;
	for (int32 i = 0; i < iterations; i++) {
		// reset bitmap contents
		memset(bitmap->Bits(), 255, bitmap->BitsLength());
		// run test
//		viewNow += test(*view, width, height, testBitmap);
//		viewNow += test_lines(*view, width, height);
//		viewNow += test_straight_lines(*view, width, height);
		viewNow += test_fill_rect(*view, width, height);
//		viewNow += test_ellipses(*view, width, height);
	}

	bitmap->Unlock();

	fprintf(stdout, " %lld µsecs\n", viewNow / iterations);
	
	if (painterNow > viewNow)
		printf("BView is %.2f times faster.\n", (float)painterNow / (float)viewNow);
	else
		printf("Painter is %.2f times faster.\n", (float)viewNow / (float)painterNow);


	BitmapView* bViewView = new BitmapView(bounds, "view", bitmap);
	bViewView->MoveTo(BPoint(bounds.left, bounds.bottom + 1));

	BWindow* window = new BWindow(BRect(50.0, 50.0, 50.0 + bounds.Width(), 50.0 + bounds.Height() * 2 + 1), "Painter Test",
								  B_TITLED_WINDOW,
								  B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);

	window->AddChild(painterView);
	window->AddChild(bViewView);

	window->Show();
	app->Run();
	delete app;
	return 0;
}
