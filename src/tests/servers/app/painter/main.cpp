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

template<class Surface>
bigtime_t
test(Surface& s,
	 uint32 width, uint32 height, BPoint origin, float scale,
	 float penSize,
	 rgb_color lineColor1, rgb_color lineColor2,
	 BRect rect, BRect roundRect,
	 rgb_color rectColorH, rgb_color rectColorL,
	 float angle, float span, rgb_color arcColor,
	 rgb_color ellipseColorH, rgb_color ellipseColorL,
	 BPoint center, float xRadius, float yRadius,
	 rgb_color roundRectColorH, rgb_color roundRectColorL,
	 BFont* font, BPoint stringLocation1, BPoint stringLocation2,
	 const char* string1, const char* string2,
	 rgb_color textColor1, rgb_color textColor2, rgb_color alphaColor,
	 BBitmap* testBitmap, BRect testBitmapCrop, BRect testBitmapDestRect)
{
	bigtime_t now = system_time();

	s.SetOrigin(origin);
	s.SetScale(scale);

	s.SetDrawingMode(B_OP_COPY);
//	s.SetDrawingMode(B_OP_SUBTRACT);
//	s.SetDrawingMode(B_OP_OVER);
	s.SetHighColor(lineColor1);
	s.SetLowColor(lineColor2);
	for (uint32 y = 0; y <= height / 2; y += 10) 
		s.StrokeLine(BPoint(0, 0), BPoint(width / 2, y)/*, kDottedBigger*/);
	for (uint32 x = 0; x <= width; x += 10)
		s.StrokeLine(BPoint(0, 0), BPoint(x, height)/*, kDottedBigger*/);
	s.SetPenSize(penSize * 5);
	s.SetHighColor(rectColorH);
	s.SetLowColor(rectColorL);
//	s.SetScale(1.0);
//	s.SetOrigin(B_ORIGIN);
//	s.SetDrawingMode(B_OP_INVERT);
	s.SetDrawingMode(B_OP_COPY);
//	s.StrokeRect(rect);
	s.StrokeRect(rect, kStripes);

//	s.ConstrainClipping(noClip);
/*	s.SetPenLocation(BPoint(230.0, 30.0));
	s.StrokeLine(BPoint(250.0, 30.0));
	s.StrokeLine(BPoint(250.0, 50.0));
	s.StrokeLine(BPoint(230.0, 50.0));
	s.StrokeLine(BPoint(230.0, 30.0));*/

	s.SetHighColor(ellipseColorH);
	s.SetLowColor(ellipseColorL);
//	s.SetDrawingMode(B_OP_OVER);
	s.SetDrawingMode(B_OP_ERASE);
	s.StrokeEllipse(center, xRadius, yRadius, kDottedBigger);
	s.SetHighColor(arcColor);
	s.SetDrawingMode(B_OP_INVERT);
	s.FillArc(center, xRadius * 2, yRadius * 2, angle, span);
//	s.StrokeArc(center, xRadius * 2, yRadius * 2, angle, span);
	s.SetDrawingMode(B_OP_OVER);
	s.SetPenSize(2.0);
	s.StrokeEllipse(center, xRadius * 3, yRadius * 2);
//	s.FillEllipse(center, xRadius * 3, yRadius * 2);
//	s.StrokeLine(bounds.RightTop());
//	s.FillRect(rect);
	s.SetHighColor(roundRectColorH);
	s.SetLowColor(roundRectColorL);
	s.SetPenSize(1.0);
//	s.SetDrawingMode(B_OP_SELECT);
	s.StrokeRoundRect(roundRect, 40, 40);
//	s.FillRoundRect(roundRect, 40, 40);

	s.SetFont(font);
	s.SetHighColor(textColor1);
	s.SetDrawingMode(B_OP_OVER);
	s.DrawString(string1, stringLocation1);
	s.DrawString(string2);
	s.StrokeLine(BPoint(width - 1, 0));

//	s.SetScale(2.5);
	s.SetDrawingMode(B_OP_INVERT);
	s.SetHighColor(textColor2);
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
//	s.SetHighColor(0, 60, 240);
//	s.DrawString("Hello Nurse!", stringLocation2);

	// bitmap drawing
	s.SetScale(1.5);
	s.SetDrawingMode(B_OP_ALPHA);
//	s.SetDrawingMode(B_OP_SELECT);
//	s.SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
	s.SetHighColor(alphaColor);
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
	BRegion noClip(bounds);

//	BRegion clip(BRect(50, 50, 100, 100));
//	clip.Include(BRect(20, 10, 80, 110));
//	clip.Include(BRect(120, -20, 180, 250));
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
	float penSize = 3.0;
	float scale = 1.0;
	rgb_color lineColor1 = (rgb_color){ 20, 20, 20, 255 };
	rgb_color lineColor2 = (rgb_color){ 220, 120, 80, 255 };
	rgb_color textColor1 = (rgb_color){ 91, 105, 98, 120 };
	rgb_color textColor2 = (rgb_color){ 200, 200, 200, 255 };
//	rgb_color textColor2 = (rgb_color){ 200, 200, 200, 255 };
	rgb_color rectColorH = (rgb_color){ 255, 0, 0, 255 };
	rgb_color rectColorL = (rgb_color){ 0, 0, 255, 255 };
	rgb_color arcColor = (rgb_color){ 255, 0, 0, 255 };
	rgb_color ellipseColorH = (rgb_color){ 255, 255, 0, 255 };
	rgb_color ellipseColorL = (rgb_color){ 128, 0, 50, 255 };
	rgb_color roundRectColorH = (rgb_color){ 0, 0, 255, 255 };
	rgb_color roundRectColorL = (rgb_color){ 255, 0, 0, 255 };
	rgb_color alphaColor = (rgb_color){ 0, 0, 0, 120 };
	
// TODO: Painter behaves differently when origin has subpixel offset
//	BPoint origin(20.3, 10.8);
	BPoint origin(20, 10);
	BRect rect(20.2, 45.6, 219.0, 139.0);
	BRect roundRect(40, 100, 250, 220.0);
	BPoint center(bounds.Width() / 2.0, bounds.Height() / 2.0);
	float xRadius = 30.0;
	float yRadius = 20.0;
	float angle = 40.0;
	float span = 230.0;
	const char* string1 = "The Quick Brown Fox...";
	const char* string2 = "jumps!";
	BPoint stringLocation1(10.0, 220.0);
	BPoint stringLocation2(30.0 / 2.5, 115.0 / 2.5);
	BFont font(be_plain_font);
	font.SetSize(12.0);
	font.SetRotation(8.0);
//	font.SetFamilyAndStyle(1);

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

	BRect testBitmapCrop(testBitmap->Bounds());
	testBitmapCrop.left += 20.0;
	BRect testBitmapDestRect = testBitmapCrop;
	testBitmapDestRect.OffsetBy(50, 20);
//	testBitmapDestRect.OffsetTo(0, 0);
//	testBitmapDestRect.right = testBitmapDestRect.left + testBitmapDestRect.Width() * 2;
//	testBitmapDestRect.bottom = testBitmapDestRect.top + testBitmapDestRect.Height() * 2;

	// frame buffer
	BBitmap* bitmap = new BBitmap(bounds, B_RGB32);
//memset(bitmap->Bits(), 0, bitmap->BitsLength());
	BitmapBuffer* buffer = new BitmapBuffer(bitmap);
	Painter painter;
	painter.AttachToBuffer(buffer);

	uint32 width = buffer->Width();
	uint32 height = buffer->Height();

//	painter.ConstrainClipping(clip);

	int32 iterations = 20;

fprintf(stdout, "Painter...");
fflush(stdout);

	bigtime_t painterNow = 0;
	for (int32 i = 0; i < iterations; i++) {
		// reset bitmap contents
		memset(bitmap->Bits(), 255, bitmap->BitsLength());
		// run test
		painterNow += test(painter,
							width, height, origin, scale, penSize,
							lineColor1, lineColor2,
							rect, roundRect,
							rectColorH, rectColorL,
							angle, span, arcColor,
							ellipseColorH, ellipseColorL,
							center, xRadius, yRadius,
							roundRectColorH, roundRectColorL,
							&font, stringLocation1, stringLocation2,
							string1, string2,
							textColor1, textColor2, alphaColor,
							testBitmap, testBitmapCrop, testBitmapDestRect);
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
		viewNow += test(*view,
						 width, height, origin, scale, penSize,
						 lineColor1, lineColor2,
						 rect, roundRect,
						 rectColorH, rectColorL,
						 angle, span, arcColor,
						 ellipseColorH, ellipseColorL,
						 center, xRadius, yRadius,
						 roundRectColorH, roundRectColorL,
						 &font, stringLocation1, stringLocation2,
						 string1, string2,
						 textColor1, textColor2, alphaColor,
						 testBitmap, testBitmapCrop, testBitmapDestRect);
	}

fprintf(stdout, " %lld µsecs\n", viewNow / iterations);

if (painterNow > viewNow)
printf("BView is %.2f times faster.\n", (float)painterNow / (float)viewNow);
else
printf("Painter is %.2f times faster.\n", (float)viewNow / (float)painterNow);

	bitmap->Unlock();

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
