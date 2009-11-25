// PatchRow.cpp
// ------------
// Implements the PatchRow class.
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#include <stdio.h>
#include <CheckBox.h>
#include <Debug.h>
#include <MidiRoster.h>
#include <MidiConsumer.h>
#include <MidiProducer.h>
#include <Window.h>
#include "MidiEventMeter.h"
#include "PatchRow.h"

extern const float ROW_LEFT = 50.0f;
extern const float ROW_TOP = 50.0f;
extern const float ROW_HEIGHT = 40.0f;
extern const float COLUMN_WIDTH = 40.0f;
extern const float METER_PADDING = 15.0f;
extern const uint32 MSG_CONNECT_REQUEST = 'mCRQ';

static const BPoint kBoxOffset(8,7);

// PatchCheckBox is the check box that describes a connection
// between a producer and a consumer.
class PatchCheckBox : public BCheckBox
{
public:
	PatchCheckBox(BRect r, int32 producerID, int32 consumerID)
		: BCheckBox(r, "", "", new BMessage(MSG_CONNECT_REQUEST))
	{
		m_producerID = producerID;
		m_consumerID = consumerID;
	}

	int32 ProducerID() const { return m_producerID; }
	int32 ConsumerID() const { return m_consumerID; }

	void DoConnect();
	
private:
	int32 m_producerID;
	int32 m_consumerID;
};

PatchRow::PatchRow(int32 producerID)
	: BView(BRect(0,0,0,0), "PatchRow", B_FOLLOW_NONE,
	B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_PULSE_NEEDED),
	m_producerID(producerID), m_eventMeter(NULL)
{
	m_eventMeter = new MidiEventMeter(m_producerID);
}

PatchRow::~PatchRow()
{
	delete m_eventMeter;
}

int32 PatchRow::ID() const
{
	return m_producerID;
}

void PatchRow::Pulse()
{
	if (m_eventMeter) m_eventMeter->Pulse(this);
}

void PatchRow::Draw(BRect)
{
	if (m_eventMeter) m_eventMeter->Draw(this);
}

void PatchRow::AddColumn(int32 consumerID)
{
	BRect r;
	int32 numColumns = CountChildren();
	r.left = numColumns*COLUMN_WIDTH + METER_PADDING + kBoxOffset.x;
	r.top = kBoxOffset.y;
	r.right = r.left + 20;
	r.bottom = r.top + 20;
	
	PatchCheckBox* box = new PatchCheckBox(r, m_producerID, consumerID);
	AddChild(box);
	box->SetTarget(this);
}

void PatchRow::RemoveColumn(int32 consumerID)
{
	int32 numChildren = CountChildren();
	for (int32 i=0; i<numChildren; i++) {
		PatchCheckBox* box = dynamic_cast<PatchCheckBox*>(ChildAt(i));
		if (box && box->ConsumerID() == consumerID) {
			RemoveChild(box);
			delete box;
			while (i < numChildren) {
				box = dynamic_cast<PatchCheckBox*>(ChildAt(i++));
				if (box) {
					box->MoveBy(-COLUMN_WIDTH, 0);
				}
			}
			break;
		}
	}
}

void PatchRow::Connect(int32 consumerID)
{
	int32 numChildren = CountChildren();
	for (int32 i=0; i<numChildren; i++) {
		PatchCheckBox* box = dynamic_cast<PatchCheckBox*>(ChildAt(i));
		if (box && box->ConsumerID() == consumerID) {
			box->SetValue(1);
		}
	}
}

void PatchRow::Disconnect(int32 consumerID)
{
	int32 numChildren = CountChildren();
	for (int32 i=0; i<numChildren; i++) {
		PatchCheckBox* box = dynamic_cast<PatchCheckBox*>(ChildAt(i));
		if (box && box->ConsumerID() == consumerID) {
			box->SetValue(0);
		}
	}
}

void PatchRow::AttachedToWindow()
{
	Window()->SetPulseRate(200000);
	SetViewColor(Parent()->ViewColor());
	int32 numChildren = CountChildren();
	for (int32 i=0; i<numChildren; i++) {
		PatchCheckBox* box = dynamic_cast<PatchCheckBox*>(ChildAt(i));
		if (box) {
			box->SetTarget(this);
		}
	}
}

void PatchRow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case MSG_CONNECT_REQUEST:
		{
			BControl* ctrl;
			if (msg->FindPointer("source", (void**) &ctrl) == B_OK) {
				PatchCheckBox* box = dynamic_cast<PatchCheckBox*>(ctrl);
				if (box) {
					box->DoConnect();
				}
			}
		}
		break;
	default:
		BView::MessageReceived(msg);
		break;
	}
}

void PatchCheckBox::DoConnect()
{
	int32 value = Value();
	int32 inverseValue = (value + 1) % 2;

	BMidiRoster* roster = BMidiRoster::MidiRoster();
	if (! roster) {
		SetValue(inverseValue);
		return;
	}
	
	BMidiProducer* producer = roster->FindProducer(m_producerID);
	BMidiConsumer* consumer = roster->FindConsumer(m_consumerID);	
	if (producer && consumer) {
		status_t err;
		if (value) {
			err = producer->Connect(consumer);
		} else {
			err = producer->Disconnect(consumer);
		}
		
		if (err != B_OK) {
			SetValue(inverseValue);
		}
	} else {
		SetValue(inverseValue);
	}
	if (producer) producer->Release();
	if (consumer) consumer->Release();
}
