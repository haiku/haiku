/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Inspired by SoundCapture from Be newsletter (Media Kit Basics:
 * 	Consumers and Producers)
 */

#include <stdio.h>
#include <string.h>
#include <IconUtils.h>
#include <MimeType.h>
#include <Screen.h>
#include <Window.h>
#include "DrawingTidbits.h"
#include "ScopeView.h"

#define SAMPLES_COUNT 20000

//#define TRACE 1
#ifdef TRACE
#define TRACE(x...) printf(x)
#else
#define TRACE(x...)
#endif

ScopeView::ScopeView(BRect rect, uint32 resizeFlags)
	: BView(rect, "scope", resizeFlags, B_WILL_DRAW | B_FRAME_EVENTS),
	fThreadId(-1),
	fBitmap(NULL),
	fBitmapView(NULL),
	fIsRendering(false),
	fMediaTrack(NULL),
	fMainTime(0),
	fRightTime(1000000),
	fLeftTime(0),
	fTotalTime(1000000)
{
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
	InitBitmap();
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
	SetHighColor(0,0,0);

	if (!fIsRendering)
		DrawBitmapAsync(fBitmap, BPoint(0, 0));
	else
		FillRect(bounds);

	float x = 0;
	if (fTotalTime != 0)
		x += (fMainTime - fLeftTime) * bounds.right
			/ (fRightTime - fLeftTime);
	SetHighColor(60,255,40);
	StrokeLine(BPoint(x, bounds.top), BPoint(x, bounds.bottom));

	Sync();
}


void
ScopeView::Run()
{
	fRenderSem = create_sem(0, "scope rendering");
	fThreadId = spawn_thread(&RenderLaunch, "Scope view", B_NORMAL_PRIORITY,
		this);
	if (fThreadId < 0)
		return;
	resume_thread(fThreadId);
}

void
ScopeView::Quit()
{
	delete_sem(fRenderSem);
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


template<typename T, typename U>
void
ScopeView::ComputeRendering()
{
	int64 framesCount = fMediaTrack->CountFrames() / SAMPLES_COUNT;
	if (framesCount <= 0)
		return;
	T samples[fPlayFormat.u.raw_audio.buffer_size
		/ (fPlayFormat.u.raw_audio.format
		& media_raw_audio_format::B_AUDIO_SIZE_MASK)];
	int64 frames = 0;
	U sum = 0;
	int64 sumCount = 0;
	float middle = fHeight / 2;
	int32 previewMax = 0;
	//fMediaTrack->SeekToFrame(&frames);

	TRACE("begin computing\n");

	int32 previewIndex = 0;

	while (fIsRendering && fMediaTrack->ReadFrames(samples, &frames) == B_OK) {
		//TRACE("reading block\n");
		int64 framesIndex = 0;

		while (framesIndex < frames) {
			for (; framesIndex < frames && sumCount < framesCount;
				framesIndex++, sumCount++) {
				sum += samples[2 * framesIndex];
				sum += samples[2 * framesIndex + 1];
			}

			if (previewIndex >= SAMPLES_COUNT)
				break;

			if (sumCount >= framesCount) {
				// TRACE("computing block %ld, sumCount %ld\n", previewIndex,
				// sumCount);
				fPreview[previewIndex] = (int32)(sum
					/ fPlayFormat.u.raw_audio.channel_count / framesCount);
				if (previewMax < fPreview[previewIndex])
					previewMax = fPreview[previewIndex];
				sumCount = 0;
				sum = 0;
				previewIndex++;
			}
		}
	}

	if (previewMax <= 0)
		return;
	for (int i = 0; i < SAMPLES_COUNT; i++)
		fPreview[i] = (int32)(fPreview[i] * 1.0 / previewMax
			* middle + middle);
}


void
ScopeView::RenderLoop()
{
	while (acquire_sem(fRenderSem) == B_OK) {
		fIsRendering = true;

		switch (fPlayFormat.u.raw_audio.format) {
			case media_raw_audio_format::B_AUDIO_FLOAT:
				ComputeRendering<float, float>();
				break;
			case media_raw_audio_format::B_AUDIO_INT:
				ComputeRendering<int32, int64>();
				break;
			case media_raw_audio_format::B_AUDIO_SHORT:
				ComputeRendering<int16, int64>();
				break;
			case media_raw_audio_format::B_AUDIO_UCHAR:
				ComputeRendering<uchar, uint32>();
				break;
			case media_raw_audio_format::B_AUDIO_CHAR:
				ComputeRendering<char, int32>();
				break;
		}

		TRACE("finished computing, rendering\n");

		/* rendering */
		RenderBitmap();

		TRACE("rendering done\n");

		/* ask drawing */

		fIsRendering = false;

		if (Window()->LockWithTimeout(5000) == B_OK) {
			Invalidate();
			TRACE("invalidate done\n");
			Window()->Unlock();
		}
	}
}


void
ScopeView::SetMainTime(bigtime_t timestamp)
{
	fMainTime = timestamp;
	Invalidate();
	TRACE("invalidate done\n");
}


void
ScopeView::SetTotalTime(bigtime_t timestamp, bool reset)
{
	fTotalTime = timestamp;
	if (reset) {
		fMainTime = 0;
		fLeftTime = 0;
		fRightTime = fTotalTime;
	}
	Invalidate();
	TRACE("invalidate done\n");
}


void
ScopeView::SetLeftTime(bigtime_t timestamp)
{
	fLeftTime = timestamp;
	RenderBitmap();
	Invalidate();
	TRACE("invalidate done\n");
}

void
ScopeView::SetRightTime(bigtime_t timestamp)
{
	fRightTime = timestamp;
	RenderBitmap();
	Invalidate();
	TRACE("invalidate done\n");
}


void
ScopeView::RenderTrack(BMediaTrack *track, const media_format &format)
{
	fMediaTrack = track;
	fPlayFormat = format;
	release_sem(fRenderSem);
}


void
ScopeView::CancelRendering()
{
	fIsRendering = false;
}


void
ScopeView::FrameResized(float width, float height)
{
	InitBitmap();
	RenderBitmap();
	Invalidate();
	TRACE("invalidate done\n");
}


void
ScopeView::MouseDown(BPoint position)
{
	if (!fMediaTrack)
		return;

	uint32 buttons;
	BPoint point;
	GetMouse(&point, &buttons);

	if (buttons & B_PRIMARY_MOUSE_BUTTON) {
		// fill the drag message
		BMessage drag(B_SIMPLE_DATA);
		drag.AddInt32("be:actions", B_COPY_TARGET);
		drag.AddString("be:clip_name", "Audio Clip");
		drag.AddString("be:types", B_FILE_MIME_TYPE);

		uint8* data;
		size_t size;

		BMimeType wavType("audio/x-wav");
		if (wavType.InitCheck() < B_OK
			|| wavType.GetIcon(&data, &size) < B_OK) {
			wavType.SetTo("audio");
			if (wavType.InitCheck() < B_OK
				|| wavType.GetIcon(&data, &size) < B_OK) {
				return;
			}
		}

		BBitmap* bitmap = new BBitmap(
			BRect(0, 0, 31, 31), 0, B_RGBA32);
		if (BIconUtils::GetVectorIcon(data, size, bitmap) < B_OK) {
			delete[] data;
			delete bitmap;
			return;
		}
		delete[] data;
		DragMessage(&drag, bitmap, B_OP_ALPHA, BPoint(0,0));
	}
}


void
ScopeView::InitBitmap()
{
	if (fBitmap != NULL && fBitmapView != NULL) {
		fBitmap->RemoveChild(fBitmapView);
		delete fBitmapView;
	}
	if (fBitmap)
		delete fBitmap;

	BRect rect = Bounds();

	fBitmap = new BBitmap(rect, BScreen().ColorSpace(), true);
	memset(fBitmap->Bits(), 0, fBitmap->BitsLength());

	rect.OffsetToSelf(B_ORIGIN);
	fBitmapView = new BView(rect.OffsetToSelf(B_ORIGIN), "bitmapView",
		B_FOLLOW_LEFT|B_FOLLOW_TOP, B_WILL_DRAW);
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
	float width = fBitmapView->Bounds().Width() + 1;

	fBitmapView->SetDrawingMode(B_OP_ADD);
	fBitmapView->SetHighColor(15,60,15);
	int32 leftIndex =
		(fTotalTime != 0) ? fLeftTime * 20000 / fTotalTime : 0;
	int32 rightIndex =
		(fTotalTime != 0) ? fRightTime * 20000 / fTotalTime : 20000;

	for (int32 i = leftIndex; i<rightIndex; i++) {
		BPoint point((i - leftIndex) * width / (rightIndex - leftIndex),
			fPreview[i]);
		//TRACE("point x %f y %f\n", point.x, point.y);
		fBitmapView->StrokeLine(point, point);
	}

	fBitmap->Unlock();
}

