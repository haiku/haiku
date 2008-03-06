/*
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#include "VideoNode.h"
#include "VideoView.h"
#include "debug.h"

#include <Bitmap.h>
#include <Locker.h>
#include <MediaRoster.h>
#include <Message.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


VideoView::VideoView(BRect frame, const char *name, uint32 resizeMask, uint32 flags, VideoNode *node)
 :	BView(frame, name, resizeMask, flags)
 ,	fVideoNode(node)
 ,	fOverlayActive(false)
{
	SetViewColor(B_TRANSPARENT_COLOR);
}


VideoView::~VideoView()
{	
}


void
VideoView::AttachedToWindow()
{
}	


VideoNode *
VideoView::Node()
{
	return fVideoNode;
}


void
VideoView::OverlayLockAcquire()
{
	CALLED();
}


void
VideoView::OverlayLockRelease()
{
	CALLED();
	// overlaybitmap->UnlockBits	
}


void
VideoView::OverlayScreenshotPrepare()
{
	CALLED();
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
}


void
VideoView::OverlayScreenshotCleanup()
{
	CALLED();
/*
	snooze(50000); // give app server some time to take the screenshot
	fVideoNode->LockBitmap();
	if (fOverlayActive) {
		BBitmap *bmp = fVideoNode->Bitmap();
		if (bmp) {
			DrawBitmap(bmp, Bounds());
			SetViewOverlay(bmp, bmp->Bounds(), Bounds(), &fOverlayKeyColor,
				B_FOLLOW_ALL, B_OVERLAY_FILTER_HORIZONTAL | B_OVERLAY_FILTER_VERTICAL);
			Invalidate();
		}
	}
	fVideoNode->UnlockBitmap();
*/
}


void
VideoView::RemoveVideoDisplay()
{
	CALLED();

	if (fOverlayActive) {
		ClearViewOverlay();
		fOverlayActive = false;
	}
	Invalidate();
}


void
VideoView::RemoveOverlay()
{
	CALLED();
	if (LockLooperWithTimeout(50000) == B_OK) {
		ClearViewOverlay();
		fOverlayActive = false;
		UnlockLooper();
	}
}


void
VideoView::Draw(BRect updateRect)
{
	if (fOverlayActive) {
		SetHighColor(fOverlayKeyColor);
		FillRect(updateRect);
	} else {
		fVideoNode->LockBitmap();
		BBitmap *bmp = fVideoNode->Bitmap();
		if (!bmp) {
			SetHighColor(0, 0, 0, 0);
			FillRect(updateRect);
		} else 
			DrawBitmap(bmp, Bounds());
		fVideoNode->UnlockBitmap();
	}
}


void
VideoView::DrawFrame()
{
	//CALLED();

	bool want_overlay = fVideoNode->IsOverlayActive();

	if (!want_overlay && fOverlayActive) {
		if (LockLooperWithTimeout(50000) == B_OK) {
			ClearViewOverlay();
			UnlockLooper();			
			fOverlayActive = false;
		} else {
			fprintf(stderr, "VideoView::DrawFrame: cannot ClearViewOverlay, as LockLooperWithTimeout failed\n");
		}
	}

	if (want_overlay && !fOverlayActive) {
		fVideoNode->LockBitmap();
		BBitmap *bmp = fVideoNode->Bitmap();
		if (bmp && LockLooperWithTimeout(50000) == B_OK) {
			SetViewOverlay(bmp, bmp->Bounds(), Bounds(), &fOverlayKeyColor,
				B_FOLLOW_ALL, B_OVERLAY_FILTER_HORIZONTAL | B_OVERLAY_FILTER_VERTICAL);
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
VideoView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {

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
		BBitmap *test = new BBitmap(BRect(0,0,320,240),	B_BITMAP_WILL_OVERLAY | B_BITMAP_RESERVE_OVERLAY_CHANNEL, colspace[i].colspace);
		if (test->InitCheck() == B_OK) {
			printf("Display supports %s (0x%08x) overlay\n", colspace[i].name, colspace[i].colspace);
			supported = true;
		}
		delete test;
//		if (supported)
//			break;
	}
	return supported;
}
