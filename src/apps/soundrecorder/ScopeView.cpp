/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Inspired by SoundCapture from Be newsletter (Media Kit Basics: Consumers and Producers)
 */

#include <stdio.h>
#include <string.h>
#include <Screen.h>
#include <Window.h>
#include "DrawingTidbits.h"
#include "ScopeView.h"

//#define TRACE 1
#ifdef TRACE
#define TRACE(x) printf(x)
#else
#define TRACE(x)
#endif

ScopeView::ScopeView(BRect rect, uint32 resizeFlags)
	: BView(rect, "scope", resizeFlags, B_WILL_DRAW | B_FRAME_EVENTS),
	fThreadId(-1),
	fBitmap(NULL),
	fIsRendering(false),
	fMediaTrack(NULL),
	fQuitting(false),
	fMainTime(0),
	fRightTime(1000000),
	fLeftTime(0),
	fTotalTime(1000000)
{
	fBitmap = new BBitmap(rect, BScreen().ColorSpace(), true);
	memset(fBitmap->Bits(), 0, fBitmap->BitsLength());
	
	rect.OffsetToSelf(B_ORIGIN);
	rect.right -= 2;
	fBitmapView = new BView(rect.OffsetToSelf(B_ORIGIN), "bitmapView", B_FOLLOW_LEFT|B_FOLLOW_TOP, B_WILL_DRAW);
	fBitmap->AddChild(fBitmapView);
	
	fRenderSem = create_sem(0, "scope rendering");
	fHeight = Bounds().Height();
}


ScopeView::~ScopeView()
{
	delete_sem(fRenderSem);
}


void
ScopeView::AttachedToWindow()
{
	SetViewColor(B_TRANSPARENT_COLOR);
	Run();
}


void
ScopeView::DetachedFromWindow()
{
	Quit();
}


void
ScopeView::Draw(BRect updateRect)
{
	BRect bounds = Bounds();
	if (!fIsRendering)
		DrawBitmapAsync(fBitmap, BPoint(2,0));
		
	float x = 2;
	if (fTotalTime !=0)
		x = 2 + (fMainTime - fLeftTime) * (bounds.right - 2) / (fRightTime - fLeftTime);
	SetHighColor(60,255,40);
	StrokeLine(BPoint(x, bounds.top), BPoint(x, bounds.bottom));
	
	Sync();
}


void
ScopeView::Run()
{
	fThreadId = spawn_thread(&RenderLaunch, "Scope view", B_NORMAL_PRIORITY, this);
	if (fThreadId < 0)
		return;
	resume_thread(fThreadId);
}

void
ScopeView::Quit()
{
	delete_sem(fRenderSem);
	fQuitting = true;
	snooze(10000);
	kill_thread(fThreadId);
}



int32
ScopeView::RenderLaunch(void *data)
{
	ScopeView *scope = (ScopeView*) data;
	scope->RenderLoop();
	return B_OK;
}


void
ScopeView::RenderLoop()
{
	while (!fQuitting) {
		if (acquire_sem(fRenderSem)!=B_OK)
			continue;

		fIsRendering = true;

		//int32 frameSize = (fPlayFormat.u.raw_audio.format & 0xf) * fPlayFormat.u.raw_audio.channel_count;
		int64 totalFrames = fMediaTrack->CountFrames();
		int16 samples[fPlayFormat.u.raw_audio.buffer_size / (fPlayFormat.u.raw_audio.format & 0xf)];
		int64 frames = 0;
		int64 sum = 0;
		int64 framesIndex = 0;
		int32 sumCount = 0;
		fMediaTrack->SeekToFrame(&frames);

		TRACE("begin computing\n");

		int32 previewIndex = 0;

		while (fMediaTrack->ReadFrames(samples, &frames) == B_OK) {
			//TRACE("reading block\n");
			framesIndex = 0;

			while (framesIndex < frames) {
				for (; framesIndex < frames && sumCount < totalFrames/20000; framesIndex++, sumCount++) {
					sum += samples[2*framesIndex];
					sum += samples[2*framesIndex+1];
				}
				
				if (previewIndex >= 20000) {
					break;
				}
				
				if (sumCount >= totalFrames/20000) {
					//TRACE("computing block %ld, sumCount %ld\n", previewIndex, sumCount);
					fPreview[previewIndex++] = (int32)(sum / 2 /(totalFrames/20000) / 32767.0 * fHeight / 2 + fHeight / 2);
					sumCount = 0;
					sum = 0;
				}
			}
			
			
		}
		
		TRACE("finished computing\n");
		
		/* rendering */
		RenderBitmap();
	
		/* ask drawing */
		
		fIsRendering = false;
	
		if (Window()->LockWithTimeout(5000) == B_OK) {
			Invalidate();
			Window()->Unlock();
		}
	}	
}


void 
ScopeView::SetMainTime(bigtime_t timestamp)
{
	fMainTime = timestamp;
	Invalidate();
}


void 
ScopeView::SetTotalTime(bigtime_t timestamp)
{
	fTotalTime = timestamp;
	Invalidate();
}

void 
ScopeView::SetLeftTime(bigtime_t timestamp)
{
	fLeftTime = timestamp;
	RenderBitmap();
	Invalidate();
}

void 
ScopeView::SetRightTime(bigtime_t timestamp)
{
	fRightTime = timestamp;
	RenderBitmap();
	Invalidate();
}


void
ScopeView::RenderTrack(BMediaTrack *track, media_format format)
{
	fMediaTrack = track;
	fPlayFormat = format;
	release_sem(fRenderSem);	
}


void
ScopeView::FrameResized(float width, float height)
{
	InitBitmap();
	RenderBitmap();
	Invalidate();
}


void
ScopeView::InitBitmap()
{
	if (fBitmapView) {
		fBitmap->RemoveChild(fBitmapView);
		delete fBitmapView;
	}
	if (fBitmap)
		delete fBitmap;
		
	BRect rect = Bounds();
	
	fBitmap = new BBitmap(rect, BScreen().ColorSpace(), true);
	memset(fBitmap->Bits(), 0, fBitmap->BitsLength());
	
	rect.OffsetToSelf(B_ORIGIN);
	rect.right -= 2;
	fBitmapView = new BView(rect.OffsetToSelf(B_ORIGIN), "bitmapView", B_FOLLOW_LEFT|B_FOLLOW_TOP, B_WILL_DRAW);
	fBitmap->AddChild(fBitmapView);
}


void
ScopeView::RenderBitmap()
{
	if (!fMediaTrack)
		return;
	
	/* rendering */
	fBitmap->Lock();
	memset(fBitmap->Bits(), 0, fBitmap->BitsLength());
	float width = fBitmapView->Bounds().Width();
	
	fBitmapView->SetDrawingMode(B_OP_ADD);
	fBitmapView->SetHighColor(15,60,15);
	int32 leftIndex = (fTotalTime != 0) ? fLeftTime * 20000 / fTotalTime : 0;
	int32 rightIndex = (fTotalTime != 0) ? fRightTime * 20000 / fTotalTime : 20000;
	
	for (int32 i = leftIndex; i<rightIndex; i++) {
		BPoint point((i - leftIndex) * width / (rightIndex - leftIndex), fPreview[i]);
		//TRACE("point x %f y %f\n", point.x, point.y);
		fBitmapView->StrokeLine(point, point);
	}
	
	fBitmap->Unlock();
}

