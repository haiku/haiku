// MidiEventMeter.cpp
// ------------------
// Implements the MidiEventMeter class.
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#include <stdio.h>
#include <MidiRoster.h>
#include <MidiProducer.h>
#include <MidiConsumer.h>
#include <View.h>
#include "CountEventConsumer.h"
#include "MidiEventMeter.h"

static const BRect METER_BOUNDS(0,0,7,31);

// If we get this number of events per pulse or greater, we will
// max out the event meter.
// Value was determined empirically based on my banging on a MIDI
// keyboard controller with a pulse of 200ms.
static const int32 METER_SCALE = 10;

MidiEventMeter::MidiEventMeter(int32 producerID)
	: m_counter(NULL), m_meterLevel(0)
{
	BMidiRoster* roster = BMidiRoster::MidiRoster();
	if (roster) {
		BMidiProducer* producer = roster->FindProducer(producerID);
		if (producer) {
			BString name;
			name << producer->Name() << " Event Meter";
			m_counter = new CountEventConsumer(name.String());
			producer->Connect(m_counter);
			producer->Release();
		}
	}
}

MidiEventMeter::~MidiEventMeter()
{
	if (m_counter) m_counter->Release();
}

void MidiEventMeter::Pulse(BView* view)
{
	int32 newLevel = m_meterLevel;
	if (m_counter) {
		newLevel = CalcMeterLevel(m_counter->CountEvents());
		m_counter->Reset();
	}
	if (newLevel != m_meterLevel) {
		m_meterLevel = newLevel;
		view->Invalidate(BRect(METER_BOUNDS).InsetBySelf(1,1));
	}
}

BRect MidiEventMeter::Bounds() const
{
	return METER_BOUNDS;
}

void MidiEventMeter::Draw(BView* view)
{
	const rgb_color METER_BLACK = { 0, 0, 0, 255 };
	const rgb_color METER_GREY = { 180, 180, 180, 255 };
	const rgb_color METER_GREEN = { 0, 255, 0, 255 };

	view->PushState();

	// draw the frame
	BPoint lt = METER_BOUNDS.LeftTop();
	BPoint rb = METER_BOUNDS.RightBottom();
	view->BeginLineArray(4);
	view->AddLine(BPoint(lt.x, lt.y), BPoint(rb.x - 1, lt.y), METER_BLACK);
	view->AddLine(BPoint(rb.x, lt.y), BPoint(rb.x, rb.y - 1), METER_BLACK);
	view->AddLine(BPoint(rb.x, rb.y), BPoint(lt.x + 1, rb.y), METER_BLACK);
	view->AddLine(BPoint(lt.x, rb.y), BPoint(lt.x, lt.y + 1), METER_BLACK);
	view->EndLineArray();
	
	// draw the cells
	BRect cell = METER_BOUNDS;
	cell.InsetBy(1,1);
	cell.bottom = cell.top + (cell.Height() + 1) / 5;
	cell.bottom--;

	const float kTintArray[] = { B_DARKEN_4_TINT, B_DARKEN_3_TINT, B_DARKEN_2_TINT, B_DARKEN_1_TINT, B_NO_TINT };
	
	for (int32 i=4; i>=0; i--)
	{
		rgb_color color;
		if (m_meterLevel > i) {
			color = tint_color(METER_GREEN, kTintArray[i]);
		} else {
			color = METER_GREY;
		}
		view->SetHighColor(color);
		view->FillRect(cell);
		cell.OffsetBy(0, cell.Height() + 1);
	}
		
	view->PopState();
}

int32 MidiEventMeter::CalcMeterLevel(int32 eventCount) const
{
	// we use an approximately logarithmic scale for determing the actual
	// drawn meter level, so that low-density event streams show up well.
	if (eventCount == 0) {
		return 0;
	} else if (eventCount < (int32)(.5*METER_SCALE)) {
		return 1;
	} else if (eventCount < (int32)(.75*METER_SCALE)) {
		return 2;
	} else if (eventCount < (int32)(.9*METER_SCALE)) {
		return 3;
	} else if (eventCount < METER_SCALE) {
		return 4;
	} else {
		return 5;
	}
}
