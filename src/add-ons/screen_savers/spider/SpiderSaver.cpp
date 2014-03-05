/*
 * Copyright 2007-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 */


#include "SpiderSaver.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Bitmap.h>
#include <Catalog.h>
#include <Message.h>

#include "Polygon.h"
#include "PolygonQueue.h"
#include "SpiderView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Screensaver Spider"


//	#pragma mark - Instantiation function


extern "C" _EXPORT BScreenSaver*
instantiate_screen_saver(BMessage* message, image_id image)
{
	return new SpiderSaver(message, image);
}


//	#pragma mark - SpiderSaver


SpiderSaver::SpiderSaver(BMessage* message, image_id id)
	:
	BScreenSaver(message, id),
	fBackBitmap(NULL),
	fBackView(NULL),
	fQueues(new PolygonQueue*[MAX_QUEUE_NUMBER]),
	fQueueNumber(20),
	fMaxPolyPoints(MAX_POLY_POINTS),
	fMaxQueueDepth(MAX_QUEUE_DEPTH),
	fColor(RED),
	fPreview(false)
{
	for (int32 i = 0; i < MAX_QUEUE_NUMBER; i++)
		fQueues[i] = NULL;

	if (message != NULL) {
		int32 value;
		if (message->FindInt32("queue number", &value) == B_OK)
			fQueueNumber = value;

		if (message->FindInt32("poly points", &value) == B_OK)
			fMaxPolyPoints = value;

		if (message->FindInt32("queue depth", &value) == B_OK)
			fMaxQueueDepth = value;

		if (message->FindInt32("color", &value) == B_OK)
			fColor = value;
	}

	srand48((long int)system_time());
}


SpiderSaver::~SpiderSaver()
{
	_Cleanup();
	delete[] fQueues;
}


void
SpiderSaver::StartConfig(BView* view)
{
	SpiderView* configView = new SpiderView(view->Bounds(), this,
		fQueueNumber, fMaxPolyPoints, fMaxQueueDepth, fColor);
	view->AddChild(configView);
}


status_t
SpiderSaver::StartSaver(BView* view, bool preview)
{
	SetTickSize(50000);

	fPreview = preview;
	fBounds = view->Bounds();
	_Init(fBounds);

	return B_OK;
}


void
SpiderSaver::StopSaver()
{
	_Cleanup();
}


void
SpiderSaver::Draw(BView* view, int32 frame)
{
	fLocker.Lock();
	for (uint32 i = 0; i < fQueueNumber; i++) {
		if (fQueues[i])
			fQueues[i]->Step();
	}
	if (fBackView) {
		if (fBackBitmap->Lock()) {
			_DrawInto(fBackView);
			fBackView->Sync();
			fBackBitmap->Unlock();
		}
		view->DrawBitmap(fBackBitmap, BPoint(0.0, 0.0));
	}
	fLocker.Unlock();
}


status_t
SpiderSaver::SaveState(BMessage* into) const
{
	if (into != NULL) {
		into->AddInt32("queue number", (int32)fQueueNumber);
		into->AddInt32("poly points", (int32)fMaxPolyPoints);
		into->AddInt32("queue depth", (int32)fMaxQueueDepth);
		into->AddInt32("color", (int32)fColor);

		return B_OK;
	}

	return B_BAD_VALUE;
}


void
SpiderSaver::SetQueueNumber(uint32 number)
{
	fLocker.Lock();
	_Cleanup();
	fQueueNumber = number;
	_Init(fBounds);
	fLocker.Unlock();
}


void
SpiderSaver::SetQueueDepth(uint32 maxDepth)
{
	fLocker.Lock();
	_Cleanup();
	fMaxQueueDepth = maxDepth;
	_Init(fBounds);
	fLocker.Unlock();
}


void
SpiderSaver::SetPolyPoints(uint32 maxPoints)
{
	fLocker.Lock();
	_Cleanup();
	fMaxPolyPoints = maxPoints;
	_Init(fBounds);
	fLocker.Unlock();
}


void
SpiderSaver::SetColor(uint32 color)
{
	fLocker.Lock();
	_Cleanup();
	fColor = color;
	_Init(fBounds);
	fLocker.Unlock();
}


//	#pragma mark - SpiderSaver private methods


void
SpiderSaver::_Init(BRect bounds)
{
	_AllocBackBitmap(bounds.Width(), bounds.Height());
	uint32 minPoints = fMaxPolyPoints / 2;
	uint32 maxPoints = fMaxPolyPoints;
	uint32 minQueueDepth = fMaxQueueDepth / 2;
	uint32 maxQueueDepth = fMaxQueueDepth;

	if (fPreview) {
		minQueueDepth /= 4;
		maxQueueDepth /= 4;
	}

	for (uint32 i = 0; i < fQueueNumber; i++) {
		fQueues[i] = new PolygonQueue(new Polygon(bounds,
			minPoints + lrand48() % (maxPoints - minPoints)),
			minQueueDepth + lrand48() % (maxQueueDepth - minQueueDepth));
	}
}


void
SpiderSaver::_Cleanup()
{
	_FreeBackBitmap();
	for (int32 i = 0; i < MAX_QUEUE_NUMBER; i++) {
		delete fQueues[i];
		fQueues[i] = NULL;
	}
}


void
SpiderSaver::_AllocBackBitmap(float width, float height)
{
	// sanity check
	if (width <= 0.0 || height <= 0.0)
		return;

	BRect b(0.0, 0.0, width, height);
	fBackBitmap = new(std::nothrow) BBitmap(b, B_RGB32, true);
	if (!fBackBitmap)
		return;

	if (fBackBitmap->IsValid()) {
		fBackView = new(std::nothrow) BView(b, 0, B_FOLLOW_NONE, B_WILL_DRAW);
		if (fBackView == NULL) {
			_FreeBackBitmap();
			fprintf(stderr,
				"SpiderSaver::_AllocBackBitmap(): view allocation failed\n");
			return;
		}
		fBackBitmap->AddChild(fBackView);
		memset(fBackBitmap->Bits(), 0, fBackBitmap->BitsLength());
	} else {
		_FreeBackBitmap();
		fprintf(stderr, "SpiderSaver::_AllocBackBitmap(): bitmap invalid\n");
	}
}


void
SpiderSaver::_FreeBackBitmap()
{
	if (fBackBitmap) {
		delete fBackBitmap;
		fBackBitmap = NULL;
		fBackView = NULL;
	}
}


void
SpiderSaver::_DrawInto(BView* view)
{
	for (uint32 i = 0; i < fQueueNumber; i++) {
		switch (fColor) {
			case GREEN:
				view->SetHighColor(1, 2, 1, 255);
				break;

			case BLUE:
				view->SetHighColor(1, 1, 2, 255);
				break;

			case YELLOW:
				view->SetHighColor(2, 2, 1, 255);
				break;

			case PURPLE:
				view->SetHighColor(2, 1, 2, 255);
				break;

			case CYAN:
				view->SetHighColor(1, 2, 2, 255);
				break;

			case GRAY:
				view->SetHighColor(2, 2, 2, 255);
				break;

			case RED:
			default:
				view->SetHighColor(2, 1, 1, 255);
				break;
		}

		if (fQueues[i] == NULL)
			continue;

		if (Polygon* p = fQueues[i]->Head()) {
			view->SetDrawingMode(B_OP_ADD);
			_DrawPolygon(p, view);
		}

		if (Polygon* p = fQueues[i]->Tail()) {
			view->SetDrawingMode(B_OP_SUBTRACT);
			_DrawPolygon(p, view);
		}
	}
}


void
SpiderSaver::_DrawPolygon(Polygon* polygon, BView* view)
{
	int32 pointCount = polygon->CountPoints();
	if (pointCount > 1) {
		BPoint p = polygon->PointAt(0);
		view->MovePenTo(p);
		for (int32 i = 1; i < pointCount; i++)
			view->StrokeLine(polygon->PointAt(i));

		view->StrokeLine(p);
	}
}
