/*
 * Copyright 2007-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 */


#include "SpiderView.h"

#include <Catalog.h>
#include <ColorMenuItem.h>
#include <ControlLook.h>
#include <MenuField.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <Slider.h>
#include <StringView.h>

#include "SpiderSaver.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SpiderView"


static const uint32 kMsgQueueNumber		= 'qunm';
static const uint32 kMsgPolyNumber		= 'plnm';
static const uint32 kMsgQueueDepth		= 'qudp';
static const uint32 kMsgColor			= 'colr';


//	#pragma - SpiderView


SpiderView::SpiderView(BRect frame, SpiderSaver* saver, uint32 queueNumber,
	uint32 maxPolyPoints, uint32 maxQueueDepth, uint32 color)
	:
	BView(frame, "spider view", B_FOLLOW_NONE, B_WILL_DRAW | B_PULSE_NEEDED),
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
	title = new BStringView(frame, B_EMPTY_STRING,
		B_TRANSLATE("for  bonefish"));
	BFont font(be_plain_font);
	font.SetShear(110.0);
	title->SetFont(&font);
	title->SetAlignment(B_ALIGN_CENTER);
	AddChild(title);

	// controls
	frame.top = 10.0f;
	frame.bottom = frame.top + viewHeight;

	// queue slider
	frame.OffsetBy(0.0f, viewHeight);
	fQueueNumberSlider = new BSlider(frame, "queue number",
		B_TRANSLATE("Max. polygon count"), new BMessage(kMsgQueueNumber),
		1, MAX_QUEUE_NUMBER);
	fQueueNumberSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fQueueNumberSlider->SetHashMarkCount((MAX_QUEUE_NUMBER - 1) / 2 + 1);
	fQueueNumberSlider->SetValue(queueNumber);
	AddChild(fQueueNumberSlider);

	// poly sliders
	frame.OffsetBy(0.0f, viewHeight);
	fPolyNumberSlider = new BSlider(frame, "poly points",
		B_TRANSLATE("Max. points per polygon"), new BMessage(kMsgPolyNumber),
		MIN_POLY_POINTS, MAX_POLY_POINTS);
	fPolyNumberSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fPolyNumberSlider->SetHashMarkCount(MAX_POLY_POINTS - MIN_POLY_POINTS + 1);
	fPolyNumberSlider->SetValue(maxPolyPoints);
	AddChild(fPolyNumberSlider);

	// queue depth slider
	frame.OffsetBy(0.0f, viewHeight);
	fQueueDepthSlider = new BSlider(frame, "queue depth",
		B_TRANSLATE("Trail depth"), new BMessage(kMsgQueueDepth),
		MIN_QUEUE_DEPTH, MAX_QUEUE_DEPTH);
	fQueueDepthSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fQueueDepthSlider->SetHashMarkCount(
		(MAX_QUEUE_DEPTH - MIN_QUEUE_DEPTH) / 4 + 1);
	fQueueDepthSlider->SetValue(maxQueueDepth);
	AddChild(fQueueDepthSlider);

	// color menu field
	BPopUpMenu* menu = new BPopUpMenu(B_TRANSLATE("Color"));
	BColorMenuItem* item;
	BMessage* message;

	// red
	message = new BMessage(kMsgColor);
	message->AddInt32("color", RED);
	item = new BColorMenuItem(B_TRANSLATE("Red"), message,
		(rgb_color){ 255, 65, 54 });
	item->SetMarked(color == RED);
	menu->AddItem(item);

	// green
	message = new BMessage(kMsgColor);
	message->AddInt32("color", GREEN);
	item = new BColorMenuItem(B_TRANSLATE("Green"), message,
		(rgb_color){ 46, 204, 64 });
	item->SetMarked(color == GREEN);
	menu->AddItem(item);

	// blue
	message = new BMessage(kMsgColor);
	message->AddInt32("color", BLUE);
	item = new BColorMenuItem(B_TRANSLATE("Blue"), message,
		(rgb_color){ 0, 116, 217 });
	item->SetMarked(color == BLUE);
	menu->AddItem(item);

	// yellow
	message = new BMessage(kMsgColor);
	message->AddInt32("color", YELLOW);
	item = new BColorMenuItem(B_TRANSLATE("Yellow"), message,
		(rgb_color){ 255, 220, 0 });
	item->SetMarked(color == YELLOW);
	menu->AddItem(item);

	// purple
	message = new BMessage(kMsgColor);
	message->AddInt32("color", PURPLE);
	item = new BColorMenuItem(B_TRANSLATE("Purple"), message,
		(rgb_color){ 177, 13, 201 });
	item->SetMarked(color == PURPLE);
	menu->AddItem(item);

	// cyan
	message = new BMessage(kMsgColor);
	message->AddInt32("color", CYAN);
	item = new BColorMenuItem(B_TRANSLATE("Cyan"), message,
		(rgb_color){ 127, 219, 255 });
	item->SetMarked(color == CYAN);
	menu->AddItem(item);

	// gray
	message = new BMessage(kMsgColor);
	message->AddInt32("color", GRAY);
	item = new BColorMenuItem(B_TRANSLATE("Gray"), message,
		(rgb_color){ 170, 170, 170 });
	item->SetMarked(color == GRAY);
	menu->AddItem(item);

	frame.OffsetBy(0.0f, viewHeight);
	fColorMenuField = new BMenuField( frame, "color", B_TRANSLATE("Color"),
		menu);
	fColorMenuField->SetDivider(fColorMenuField->StringWidth(
		B_TRANSLATE("Color")) + be_control_look->DefaultItemSpacing());
	AddChild(fColorMenuField);
}


SpiderView::~SpiderView()
{
}


void
SpiderView::AttachedToWindow()
{
	fQueueNumberSlider->SetTarget(this);
	fPolyNumberSlider->SetTarget(this);
	fQueueDepthSlider->SetTarget(this);
	fColorMenuField->Menu()->SetTargetForItems(this);
}


void
SpiderView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgQueueNumber:
			fSaver->SetQueueNumber(fQueueNumberSlider->Value());
			break;

		case kMsgPolyNumber:
			fSaver->SetPolyPoints(fPolyNumberSlider->Value());
			break;

		case kMsgQueueDepth:
			fSaver->SetQueueDepth(fQueueDepthSlider->Value());
			break;

		case kMsgColor: {
			uint32 color;
			if (message->FindInt32("color", (int32*)&color) == B_OK)
				fSaver->SetColor(color);

			break;
		}

		default:
			BView::MessageReceived(message);
	}
}
