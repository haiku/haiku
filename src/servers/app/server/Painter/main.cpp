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
// TODO: Painter behaves differently when origin has subpixel offset
//	BPoint origin(20.3, 10.8);
	BPoint origin(20, 10);
	BRect rect(20.2, 45.6, 219.0, 139.0);
	BRect rect2(40, 100, 250, 220.0);
	BPoint center(bounds.Width() / 2.0, bounds.Height() / 2.0);
	float xRadius = 30.0;
	float yRadius = 20.0;
	float angle = 40.0;
	float span = 230.0;
	const char* string = "The Quick Brown Fox...";
	BPoint stringLocation(50.0, 200.0);
	BFont font(be_plain_font);
	font.SetSize(20.0);
	font.SetRotation(15.0);
//	font.SetFamilyAndStyle(1);

	BBitmap* testBitmap = new BBitmap(BRect(20, 5, 72, 46), 0, B_RGB32);
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
			h[3] = 255;
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
//	testBitmapCrop.left += 20.0;
	BRect testBitmapDestRect = testBitmapCrop;
	testBitmapDestRect.OffsetBy(200, 20);
	testBitmapDestRect.right = testBitmapDestRect.left + testBitmapDestRect.Width() * 2;
	testBitmapDestRect.bottom = testBitmapDestRect.top + testBitmapDestRect.Height() * 2;

	// frame buffer
	BBitmap* bitmap = new BBitmap(bounds, B_RGB32);
	BitmapBuffer* buffer = new BitmapBuffer(bitmap);
	Painter painter;
	painter.AttachToBuffer(buffer);

	uint32 width = buffer->Width();
	uint32 height = buffer->Height();

	painter.ConstrainClipping(clip);
	painter.SetScale(scale);
//	painter.SetPenSize(penSize);
	painter.SetOrigin(origin);

bigtime_t painterNow = system_time();

	painter.SetDrawingMode(B_OP_OVER);
	painter.SetHighColor(210, 210, 210);
	for (uint32 y = 0; y <= height / 2; y += 10) 
		painter.StrokeLine(BPoint(0, 0), BPoint(width / 2, y));
	for (uint32 x = 0; x <= width; x += 10)
		painter.StrokeLine(BPoint(0, 0), BPoint(x, height));
	painter.SetPenSize(penSize * 5);
	painter.SetHighColor(255, 0, 0, 255);
	painter.SetLowColor(128, 0, 255, 255);
//	painter.SetScale(1.0);
//	painter.SetOrigin(B_ORIGIN);
//	painter.SetDrawingMode(B_OP_INVERT);
	painter.SetDrawingMode(B_OP_COPY);
//	painter.StrokeRect(rect);
	painter.StrokeRect(rect, kStripes);

//	painter.ConstrainClipping(noClip);
/*	painter.SetPenLocation(BPoint(230.0, 30.0));
	painter.StrokeLine(BPoint(250.0, 30.0));
	painter.StrokeLine(BPoint(250.0, 50.0));
	painter.StrokeLine(BPoint(230.0, 50.0));
	painter.StrokeLine(BPoint(230.0, 30.0));*/

	painter.SetHighColor(255, 255, 0, 255);
	painter.SetLowColor(128, 0, 50, 255);
	painter.SetDrawingMode(B_OP_OVER);
	painter.StrokeEllipse(center, xRadius, yRadius, kDottedBigger);
	painter.SetHighColor(255, 0, 0, 255);
	painter.SetDrawingMode(B_OP_INVERT);
	painter.FillArc(center, xRadius * 2, yRadius * 2, angle, span);
//	painter.StrokeArc(center, xRadius * 2, yRadius * 2, angle, span);
	painter.SetDrawingMode(B_OP_OVER);
	painter.SetPenSize(2.0);
	painter.StrokeEllipse(center, xRadius * 3, yRadius * 2);
//	painter.FillEllipse(center, xRadius * 3, yRadius * 2);
//	painter.StrokeLine(bounds.RightTop());
//	painter.FillRect(rect);
	painter.SetHighColor(0, 0, 255, 255);
	painter.SetPenSize(1.0);
	painter.StrokeRoundRect(rect2, 40, 40);
//	painter.FillRoundRect(rect2, 40, 40);

	painter.SetFont(font);
	painter.SetHighColor(0, 20, 80, 255);
	painter.SetDrawingMode(B_OP_OVER);
	painter.DrawString(string, stringLocation);

	// bitmap drawing
//	painter.SetScale(0.5);
	painter.DrawBitmap(testBitmap, testBitmapCrop, testBitmapDestRect);


painterNow = system_time() - painterNow;
printf("Painter: %lld µsecs\n", painterNow);

	BitmapView* painterView = new BitmapView(bounds, "view", bitmap);

	bitmap = new BBitmap(bounds, B_RGB32, true);
	BView* view = new BView(bounds, NULL, B_FOLLOW_NONE, B_WILL_DRAW);
	bitmap->Lock();
	bitmap->AddChild(view);

	view->ConstrainClippingRegion(&clip);
	view->SetScale(scale);
//	view->SetPenSize(penSize);
	view->SetOrigin(origin);

bigtime_t viewNow = system_time();

	view->SetDrawingMode(B_OP_OVER);
	view->SetHighColor(210, 210, 210);
	for (uint32 y = 0; y <= height / 2; y += 10) 
		view->StrokeLine(BPoint(0, 0), BPoint(width / 2, y));
	for (uint32 x = 0; x <= width; x += 10)
		view->StrokeLine(BPoint(0, 0), BPoint(x, height));
	view->SetPenSize(penSize * 5);
	view->SetHighColor(255, 0, 0, 255);
	view->SetLowColor(128, 0, 255, 255);
//	view->SetScale(1.0);
//	view->SetOrigin(B_ORIGIN);
//	view->SetDrawingMode(B_OP_INVERT);
	view->SetDrawingMode(B_OP_COPY);
//	view->StrokeRect(rect);
	view->StrokeRect(rect, kStripes);

//	view->ConstrainClippingRegion(&noClip);
/*	view->MovePenTo(BPoint(230.0, 30.0));
	view->StrokeLine(BPoint(250.0, 30.0));
	view->StrokeLine(BPoint(250.0, 50.0));
	view->StrokeLine(BPoint(230.0, 50.0));
	view->StrokeLine(BPoint(230.0, 30.0));*/

	view->SetHighColor(255, 255, 0, 255);
	view->SetLowColor(128, 0, 50, 255);
	view->SetDrawingMode(B_OP_OVER);
	view->StrokeEllipse(center, xRadius, yRadius, kDottedBigger);
	view->SetDrawingMode(B_OP_INVERT);
	view->SetHighColor(255, 0, 0, 255);
	view->FillArc(center, xRadius * 2, yRadius * 2, angle, span);
//	view->StrokeArc(center, xRadius * 2, yRadius * 2, angle, span);
	view->SetDrawingMode(B_OP_OVER);
	view->SetPenSize(2.0);
	view->StrokeEllipse(center, xRadius * 3, yRadius * 2);
//	view->FillEllipse(center, xRadius * 3, yRadius * 2);
//	view->StrokeLine(bounds.RightTop());
//	view->FillRect(rect);
	view->SetHighColor(0, 0, 255, 255);
	view->SetPenSize(1.0);
	view->StrokeRoundRect(rect2, 40, 40);
//	view->FillRoundRect(rect2, 40, 40);

	// text drawing
	view->SetFont(&font);
	view->SetHighColor(0, 20, 80, 255);
	view->SetDrawingMode(B_OP_OVER);
	view->DrawString(string, stringLocation);

	// bitmap drawing
//	view->SetScale(0.5);
	view->DrawBitmap(testBitmap, testBitmapCrop, testBitmapDestRect);

	view->Sync();

viewNow = system_time() - viewNow;
printf("BView: %lld µsecs\n", viewNow);
printf("BView is %lld times faster.\n", painterNow / viewNow);

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
