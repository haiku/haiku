/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <Bitmap.h>
#include <MediaRoster.h>
#include <MessageRunner.h>
#include "VideoView.h"
#include "VideoNode.h"
#include "ConvertBitmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK_ACTIVITY 'ChkA'

VideoView::VideoView(BRect frame, const char *name, uint32 resizeMask, 
	uint32 flags)
 :	BView(frame, name, resizeMask, flags)
 ,	fVideoNode(0)
 ,	fVideoActive(false)
 ,	fOverlayActive(false)
 ,	fActivityCheckMsgRunner(0)
 ,	fLastFrame(0)
{
	SetViewColor(B_TRANSPARENT_COLOR);

	status_t err = B_OK;
	BMediaRoster *mroster = BMediaRoster::Roster(&err);
	if (!mroster || err) {
		printf("VideoView::VideoView: media_server is dead\n");
		exit(1);
	} else {
		fVideoNode = new VideoNode("video in", this);
		err = mroster->RegisterNode(fVideoNode);
	}
}


VideoView::~VideoView()
{
	delete fActivityCheckMsgRunner;
	
	if (fVideoNode) {
		BMediaRoster::Roster()->UnregisterNode(fVideoNode);
		delete fVideoNode;
	}
}


void
VideoView::AttachedToWindow()
{
	BMessage msg(CHECK_ACTIVITY);
	fActivityCheckMsgRunner = new BMessageRunner(BMessenger(this), &msg, 
		200000);
}	


VideoNode *
VideoView::Node()
{
	return fVideoNode;
}


void
VideoView::OverlayLockAcquire()
{
   printf("VideoView::OverlayLockAcquire\n");
}


void
VideoView::OverlayLockRelease()
{ /* [19:54] <Francois> Rudolf forwarded me a mail once about it
   * [19:55] <Francois> when you get relmease msg you are supposed to 
   				UnlockBits() on the overlay bitmaps you use
   * [19:55] <Francois> it's used when switching workspaces
   * [19:55] <Francois> as the bits might get relocated
   */
   printf("VideoView::OverlayLockRelease\n");

}


void
VideoView::OverlayScreenshotPrepare()
{
	printf("OverlayScreenshotPrepare enter\n");
/*
	fVideoNode->LockBitmap();
	if (fOverlayActive) {
		BBitmap *bmp = fVideoNode->Bitmap();
		if (bmp) {
//			Window()->UpdateIfNeeded();
//			Sync();
			BBitmap *tmp = new BBitmap(bmp->Bounds(), 0, B_RGB32);
//			ConvertBitmap(tmp, bmp);
			ClearViewOverlay();
			DrawBitmap(tmp, Bounds());
			delete tmp;
//			Sync();
		}
	}
	fVideoNode->UnlockBitmap();
*/
	printf("OverlayScreenshotPrepare leave\n");
}


void
VideoView::OverlayScreenshotCleanup()
{
	printf("OverlayScreenshotCleanup enter\n");
/*
	snooze(50000); // give app server some time to take the screenshot
	fVideoNode->LockBitmap();
	if (fOverlayActive) {
		BBitmap *bmp = fVideoNode->Bitmap();
		if (bmp) {
			DrawBitmap(bmp, Bounds());
			SetViewOverlay(bmp, bmp->Bounds(), Bounds(), &fOverlayKeyColor,
				B_FOLLOW_ALL, B_OVERLAY_FILTER_HORIZONTAL 
				| B_OVERLAY_FILTER_VERTICAL);
			Invalidate();
		}
	}
	fVideoNode->UnlockBitmap();
*/
	printf("OverlayScreenshotCleanup leave\n");
}


void
VideoView::RemoveVideoDisplay()
{
	printf("VideoView::RemoveVideoDisplay\n");
	
	if (fOverlayActive) {
		ClearViewOverlay();
		fOverlayActive = false;
	}
	fVideoActive = false;
	Invalidate();
}


void
VideoView::RemoveOverlay()
{
	printf("VideoView::RemoveOverlay\n");
	if (LockLooperWithTimeout(50000) == B_OK) {
		ClearViewOverlay();
		fOverlayActive = false;
		UnlockLooper();
	}
}


void
VideoView::CheckActivity()
{
	if (!fVideoActive)
		return;
	if (system_time() - fLastFrame < 700000)
		return;

	printf("VideoView::CheckActivity: lag detected\n");

	fVideoActive = false;
	Invalidate();
}


void
VideoView::Draw(BRect updateRect)
{
	if (!fVideoActive) {
		DrawTestImage();
		return;
	}
	if (fOverlayActive) {
		SetHighColor(fOverlayKeyColor);
		FillRect(updateRect);
	} else {
		fVideoNode->LockBitmap();
		BBitmap *bmp = fVideoNode->Bitmap();
		if (bmp)
			DrawBitmap(bmp, Bounds());
		fVideoNode->UnlockBitmap();
	}
}


void
VideoView::DrawFrame()
{
//	printf("VideoView::DrawFrame\n");
	if (!fVideoActive) {
		fVideoActive = true;
		if (LockLooperWithTimeout(50000) != B_OK)
			return;
		Invalidate();
		UnlockLooper();
	}
	fLastFrame = system_time();
	
	bool want_overlay = fVideoNode->IsOverlayActive();

	if (!want_overlay && fOverlayActive) {
		if (LockLooperWithTimeout(50000) == B_OK) {
			ClearViewOverlay();
			UnlockLooper();			
			fOverlayActive = false;
		} else {
			printf("can't ClearViewOverlay, as LockLooperWithTimeout "
				"failed\n");
		}
	}
	if (want_overlay && !fOverlayActive) {
		fVideoNode->LockBitmap();
		BBitmap *bmp = fVideoNode->Bitmap();
		if (bmp && LockLooperWithTimeout(50000) == B_OK) {
			SetViewOverlay(bmp, bmp->Bounds(), Bounds(), &fOverlayKeyColor,
				B_FOLLOW_ALL, B_OVERLAY_FILTER_HORIZONTAL 
				| B_OVERLAY_FILTER_VERTICAL);
			fOverlayActive = true;

			Invalidate();
			UnlockLooper();
		}
		fVideoNode->UnlockBitmap();
	}
	if (!fOverlayActive) {
		if (LockLooperWithTimeout(50000) != B_OK)
			return;
		Invalidate();
		UnlockLooper();
	}
}


void
VideoView::DrawTestImage()
{
	static const rgb_color cols[8] = { 
		{255,255,255}, {255,255,0}, {0,255,255}, {0,255,0},
		{255,0,255}, {255,0,0}, {0,0,255}, {0,0,0} 
//		{255,255,255}, {255,255,0}, {0,255,255}, 
//		{255,0,255}, {255,0,0}, {0,255,0}, {0,0,255}, {0,0,0} 
	};
	float bar_width;
	float left;
	float right;

	BRect bnd = Bounds();
	int seperator_y1 = int(0.60 * (bnd.Height() + 1));
	int seperator_y2 = int(0.80 * (bnd.Height() + 1));
	int steps;

	bar_width = bnd.Width() / 8;
	if (bar_width < 1)
		bar_width = 1;
	
	left = 0;
	for (int i = 0; i < 8; i++) {
		SetHighColor(cols[i]);
		right = (i != 7) ? left + bar_width - 1 : bnd.right;
		FillRect(BRect(left, 0, right, seperator_y1));
		left = right + 1;
	}
	
	steps = 32;
	
	bar_width = bnd.Width() / steps;
//	if (bar_width < 1)
//		bar_width = 1;

	left = 0;
	for (int i = 0; i < steps; i++) {
		uint8 c = i * 255 / (steps - 1);
		SetHighColor(c, c, c);
		right = (i != steps - 1) ? left + bar_width - 1 : bnd.right;
		FillRect(BRect(left, seperator_y1 + 1, right, seperator_y2));
		left = right + 1;
	}

	steps = 256;
	
	bar_width = bnd.Width() / steps;
	if (bar_width < 1)
		bar_width = 1;

	left = 0;
	for (int i = 0; i < steps; i++) {
		uint8 c = 255 - (i * 255 / (steps - 1));
		SetHighColor(c, c, c);
		right = (i != steps - 1) ? left + bar_width - 1 : bnd.right;
		FillRect(BRect(left, seperator_y2 + 1, right, bnd.bottom));
		left = right + 1;
	}
}


void
VideoView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case CHECK_ACTIVITY:
			CheckActivity();
			break;
		default:
			BView::MessageReceived(msg);
	}
}


bool
VideoView::IsOverlaySupported()
{
	struct colorcombo {
		color_space colspace;
		const char *name;
	} colspace[] = {
		{ B_RGB32,		"B_RGB32"},
		{ B_RGBA32,		"B_RGBA32"},
		{ B_RGB24,		"B_RGB24"},
		{ B_RGB16,		"B_RGB16"},
		{ B_RGB15,		"B_RGB15"},
		{ B_RGBA15,		"B_RGBA15"},
		{ B_RGB32_BIG,	"B_RGB32_BIG"},
		{ B_RGBA32_BIG,	"B_RGBA32_BIG "},
		{ B_RGB24_BIG,	"B_RGB24_BIG "},
		{ B_RGB16_BIG,	"B_RGB16_BIG "},
		{ B_RGB15_BIG,	"B_RGB15_BIG "},
		{ B_RGBA15_BIG, "B_RGBA15_BIG "},
		{ B_YCbCr422,	"B_YCbCr422"},
		{ B_YCbCr411,	"B_YCbCr411"},
		{ B_YCbCr444,	"B_YCbCr444"},
		{ B_YCbCr420,	"B_YCbCr420"},
		{ B_YUV422,		"B_YUV422"},
		{ B_YUV411,		"B_YUV411"},
		{ B_YUV444,		"B_YUV444"},
		{ B_YUV420,		"B_YUV420"},
		{ B_NO_COLOR_SPACE, NULL}
	};
	
	bool supported = false;
	for (int i = 0; colspace[i].name; i++) {
		BBitmap *test = new BBitmap(BRect(0,0,320,240),	B_BITMAP_WILL_OVERLAY 
			| B_BITMAP_RESERVE_OVERLAY_CHANNEL, colspace[i].colspace);
		if (test->InitCheck() == B_OK) {
			printf("Display supports %s (0x%08x) overlay\n", colspace[i].name, 
				colspace[i].colspace);
			overlay_restrictions restrict;
			if (B_OK == test->GetOverlayRestrictions(&restrict)) {
				printf(
					"Overlay restrictions: source horizontal_alignment %d\n", 
					restrict.source.horizontal_alignment);
				printf("Overlay restrictions: source vertical_alignment %d\n", 
					restrict.source.vertical_alignment);
				printf("Overlay restrictions: source width_alignment %d\n", 
					restrict.source.width_alignment);
				printf("Overlay restrictions: source height_alignment %d\n", 
					restrict.source.height_alignment);
				printf("Overlay restrictions: source min_width %d\n", 
					restrict.source.min_width);
				printf("Overlay restrictions: source max_width %d\n", 
					restrict.source.max_width);
				printf("Overlay restrictions: source min_height %d\n", 
					restrict.source.min_height);
				printf("Overlay restrictions: source max_height %d\n", 
					restrict.source.max_height);
				printf(
					"Overlay restrictions: destination horizontal_alignment "
					"%d\n", restrict.destination.horizontal_alignment);
				printf("Overlay restrictions: destination vertical_alignment "
					"%d\n", restrict.destination.vertical_alignment);
				printf("Overlay restrictions: destination width_alignment "
					"%d\n", restrict.destination.width_alignment);
				printf("Overlay restrictions: destination height_alignment "
					"%d\n", restrict.destination.height_alignment);
				printf("Overlay restrictions: destination min_width %d\n", 
					restrict.destination.min_width);
				printf("Overlay restrictions: destination max_width %d\n", 
					restrict.destination.max_width);
				printf("Overlay restrictions: destination min_height %d\n", 
					restrict.destination.min_height);
				printf("Overlay restrictions: destination max_height %d\n", 
					restrict.destination.max_height);
				printf("Overlay restrictions: min_width_scale %.3f\n", 
					restrict.min_width_scale);
				printf("Overlay restrictions: max_width_scale %.3f\n", 
					restrict.max_width_scale);
				printf("Overlay restrictions: min_height_scale %.3f\n", 
					restrict.min_height_scale);
				printf("Overlay restrictions: max_height_scale %.3f\n", 
					restrict.max_height_scale);
			}
			supported = true;
		}
		delete test;
//		if (supported)
//			break;
	}
	return supported;
}

