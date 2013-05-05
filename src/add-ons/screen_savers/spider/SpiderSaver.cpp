/*
 * Copyright 2007-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#include "SpiderSaver.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Bitmap.h>
#include <Catalog.h>
#include <Message.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Slider.h>
#include <StringView.h>

#include "Polygon.h"
#include "PolygonQueue.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Screensaver Spider"

enum {
	MSG_QUEUE_NUMBER			= 'qunm',
	MSG_POLY_NUMBER				= 'plnm',
	MSG_QUEUE_DEPTH				= 'qudp',
	MSG_COLOR					= 'colr',
};

#define MIN_POLY_POINTS 3
#define MAX_POLY_POINTS 10
#define MIN_QUEUE_DEPTH 40
#define MAX_QUEUE_DEPTH 160
#define MAX_QUEUE_NUMBER 40

enum {
	RED							= 1,
	GREEN						= 2,
	BLUE						= 3,
	YELLOW						= 4,
	PURPLE						= 5,
	CYAN						= 6,
	GRAY						= 7,
};

// MAIN INSTANTIATION FUNCTION
extern "C" _EXPORT BScreenSaver*
instantiate_screen_saver(BMessage *message, image_id image)
{
	return new SpiderSaver(message, image);
}

// constructor
SpiderSaver::SpiderSaver(BMessage *message, image_id id)
	: BScreenSaver(message, id),
	  fBackBitmap(NULL),
	  fBackView(NULL),
	  fQueues(new PolygonQueue*[MAX_QUEUE_NUMBER]),
	  fQueueNumber(20),
	  fMaxPolyPoints(MAX_POLY_POINTS),
	  fMaxQueueDepth(MAX_QUEUE_DEPTH),
	  fColor(RED)
{
	for (int32 i = 0; i < MAX_QUEUE_NUMBER; i++)
		fQueues[i] = NULL;
	if (message) {
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

// destructor
SpiderSaver::~SpiderSaver()
{
	_Cleanup();
	delete[] fQueues;
}

// StartConfig
void
SpiderSaver::StartConfig(BView *view)
{
	SpiderView* configView = new SpiderView(view->Bounds(), this,
		fQueueNumber, fMaxPolyPoints, fMaxQueueDepth, fColor);
	view->AddChild(configView);
}

// StartSaver
status_t
SpiderSaver::StartSaver(BView *v, bool preview)
{
	SetTickSize(50000);

	fPreview = preview;
	fBounds = v->Bounds();
	_Init(fBounds);

	return B_OK;
}

// StopSaver
void
SpiderSaver::StopSaver()
{
	_Cleanup();
}

// Draw
void
SpiderSaver::Draw(BView *view, int32 frame)
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

// SaveState
status_t
SpiderSaver::SaveState(BMessage* into) const
{
	if (into) {
		into->AddInt32("queue number", (int32)fQueueNumber);
		into->AddInt32("poly points", (int32)fMaxPolyPoints);
		into->AddInt32("queue depth", (int32)fMaxQueueDepth);
		into->AddInt32("color", (int32)fColor);
		return B_OK;
	}
	return B_BAD_VALUE;
}

// SetQueueNumber
void
SpiderSaver::SetQueueNumber(uint32 number)
{
	fLocker.Lock();
	_Cleanup();
	fQueueNumber = number;
	_Init(fBounds);
	fLocker.Unlock();
}

// SetQueueDepth
void
SpiderSaver::SetQueueDepth(uint32 maxDepth)
{
	fLocker.Lock();
	_Cleanup();
	fMaxQueueDepth = maxDepth;
	_Init(fBounds);
	fLocker.Unlock();
}

// SetPolyPoints
void
SpiderSaver::SetPolyPoints(uint32 maxPoints)
{
	fLocker.Lock();
	_Cleanup();
	fMaxPolyPoints = maxPoints;
	_Init(fBounds);
	fLocker.Unlock();
}

// SetColor
void
SpiderSaver::SetColor(uint32 color)
{
	fLocker.Lock();
	_Cleanup();
	fColor = color;
	_Init(fBounds);
	fLocker.Unlock();
}

// _Init
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
	for (uint32 i = 0; i < fQueueNumber; i++)
		fQueues[i] = new PolygonQueue(new Polygon(bounds, minPoints
															+ lrand48()
															% (maxPoints
															- minPoints)),
									  minQueueDepth + lrand48() % (maxQueueDepth
									  - minQueueDepth));
}

// _Cleanup
void
SpiderSaver::_Cleanup()
{
	_FreeBackBitmap();
	for (int32 i = 0; i < MAX_QUEUE_NUMBER; i++) {
		delete fQueues[i];
		fQueues[i] = NULL;
	}
}

// _AllocBackBitmap
void
SpiderSaver::_AllocBackBitmap(float width, float height)
{
	// sanity check
	if (width <= 0.0 || height <= 0.0)
		return;

	BRect b(0.0, 0.0, width, height);
	fBackBitmap = new BBitmap(b, B_RGB32, true);
	if (!fBackBitmap)
		return;
	if (fBackBitmap->IsValid()) {
		fBackView = new BView(b, 0, B_FOLLOW_NONE, B_WILL_DRAW);
		fBackBitmap->AddChild(fBackView);
		memset(fBackBitmap->Bits(), 0, fBackBitmap->BitsLength());
	} else {
		_FreeBackBitmap();
		fprintf(stderr, "SpiderSaver::_AllocBackBitmap(): bitmap invalid\n");
	}
}

// _FreeBackBitmap
void
SpiderSaver::_FreeBackBitmap()
{
	if (fBackBitmap) {
		delete fBackBitmap;
		fBackBitmap = NULL;
		fBackView = NULL;
	}
}

// _DrawInto
void
SpiderSaver::_DrawInto(BView *view)
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

// _DrawPolygon
void
SpiderSaver::_DrawPolygon(Polygon* polygon, BView *view)
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

// constructor
SpiderView::SpiderView(BRect frame, SpiderSaver* saver,
					   uint32 queueNumber, uint32 maxPolyPoints,
					   uint32 maxQueueDepth, uint32 color)
	: BView(frame, "spider view", B_FOLLOW_NONE, B_WILL_DRAW | B_PULSE_NEEDED),
	  fSaver(saver)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	frame.OffsetTo(0.0, 0.0);
	frame.InsetBy(10.0, 5.0);

	float viewHeight = floorf(frame.Height() / 5.0);

	// title stuff
	font_height fh;
	be_bold_font->GetHeight(&fh);
	float fontHeight = fh.ascent + fh.descent + 5.0;
	frame.bottom = frame.top + fontHeight;
	BStringView* title = new BStringView(frame, B_EMPTY_STRING,
		B_TRANSLATE("Spider by stippi"));
	title->SetFont(be_bold_font);
	AddChild(title);

	be_plain_font->GetHeight(&fh);
	fontHeight = fh.ascent + fh.descent + 5.0;
	frame.top = frame.bottom;
	frame.bottom = frame.top + fontHeight;
	title = new BStringView(frame, B_EMPTY_STRING, B_TRANSLATE("for  bonefish"));
	BFont font(be_plain_font);
	font.SetShear(110.0);
	title->SetFont(&font);
	title->SetAlignment(B_ALIGN_CENTER);
	AddChild(title);

	// controls
	frame.top = 10.0;
	frame.bottom = frame.top + viewHeight;
	frame.OffsetBy(0.0, viewHeight);
	fQueueNumberS = new BSlider(frame, "queue number",
		B_TRANSLATE("Max. polygon count"), new BMessage(MSG_QUEUE_NUMBER),
		1, MAX_QUEUE_NUMBER);
	fQueueNumberS->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fQueueNumberS->SetHashMarkCount((MAX_QUEUE_NUMBER - 1) / 2 + 1);
	fQueueNumberS->SetValue(queueNumber);
	AddChild(fQueueNumberS);
	frame.OffsetBy(0.0, viewHeight);
	fPolyNumberS = new BSlider(frame, "poly points",
		B_TRANSLATE("Max. points per polygon"), new BMessage(MSG_POLY_NUMBER),
		MIN_POLY_POINTS, MAX_POLY_POINTS);
	fPolyNumberS->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fPolyNumberS->SetHashMarkCount(MAX_POLY_POINTS - MIN_POLY_POINTS + 1);
	fPolyNumberS->SetValue(maxPolyPoints);
	AddChild(fPolyNumberS);
	frame.OffsetBy(0.0, viewHeight);
	fQueueDepthS = new BSlider(frame, "queue depth", B_TRANSLATE("Trail depth"),
		new BMessage(MSG_QUEUE_DEPTH), MIN_QUEUE_DEPTH, MAX_QUEUE_DEPTH);
	fQueueDepthS->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fQueueDepthS->SetHashMarkCount((MAX_QUEUE_DEPTH - MIN_QUEUE_DEPTH) / 4 + 1);
	fQueueDepthS->SetValue(maxQueueDepth);
	AddChild(fQueueDepthS);

	BMenu* menu = new BMenu(B_TRANSLATE("Color"));
	BMessage* message = new BMessage(MSG_COLOR);
	message->AddInt32("color", RED);
	BMenuItem* item = new BMenuItem(B_TRANSLATE("Red"), message);
	if (color == RED)
		item->SetMarked(true);
	menu->AddItem(item);
	message = new BMessage(MSG_COLOR);
	message->AddInt32("color", GREEN);
	item = new BMenuItem(B_TRANSLATE("Green"), message);
	if (color == GREEN)
		item->SetMarked(true);
	menu->AddItem(item);
	message = new BMessage(MSG_COLOR);
	message->AddInt32("color", BLUE);
	item = new BMenuItem(B_TRANSLATE("Blue"), message);
	if (color == BLUE)
		item->SetMarked(true);
	menu->AddItem(item);
	message = new BMessage(MSG_COLOR);
	message->AddInt32("color", YELLOW);
	item = new BMenuItem(B_TRANSLATE("Yellow"), message);
	if (color == YELLOW)
		item->SetMarked(true);
	menu->AddItem(item);
	message = new BMessage(MSG_COLOR);
	message->AddInt32("color", PURPLE);
	item = new BMenuItem(B_TRANSLATE("Purple"), message);
	if (color == PURPLE)
		item->SetMarked(true);
	menu->AddItem(item);
	message = new BMessage(MSG_COLOR);
	message->AddInt32("color", CYAN);
	item = new BMenuItem(B_TRANSLATE("Cyan"), message);
	if (color == CYAN)
		item->SetMarked(true);
	menu->AddItem(item);
	message = new BMessage(MSG_COLOR);
	message->AddInt32("color", GRAY);
	item = new BMenuItem(B_TRANSLATE("Gray"), message);
	if (color == GRAY)
		item->SetMarked(true);
	menu->AddItem(item);

	menu->SetLabelFromMarked(true);
	menu->SetRadioMode(true);

	frame.OffsetBy(0.0, viewHeight);
	fColorMF = new BMenuField(frame, "color", B_TRANSLATE("Color"), menu);
	fColorMF->SetDivider(fColorMF->StringWidth(B_TRANSLATE("Color")) + 5.0);
	AddChild(fColorMF);
}

// destructor
SpiderView::~SpiderView()
{
}

// AttachedToWindow
void
SpiderView::AttachedToWindow()
{
	fQueueNumberS->SetTarget(this);
	fPolyNumberS->SetTarget(this);
	fQueueDepthS->SetTarget(this);
	fColorMF->Menu()->SetTargetForItems(this);
}

// MessageReceived
void
SpiderView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_QUEUE_NUMBER:
			fSaver->SetQueueNumber(fQueueNumberS->Value());
			break;
		case MSG_POLY_NUMBER:
			fSaver->SetPolyPoints(fPolyNumberS->Value());
			break;
		case MSG_QUEUE_DEPTH:
			fSaver->SetQueueDepth(fQueueDepthS->Value());
			break;
		case MSG_COLOR: {
			uint32 color;
			if (message->FindInt32("color", (int32*)&color) == B_OK)
				fSaver->SetColor(color);
			break;
		}
		default:
			BView::MessageReceived(message);
			break;
	}
}

