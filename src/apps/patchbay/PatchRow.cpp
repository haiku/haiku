/* PatchRow.cpp
 * ------------
 *
 * Copyright 2013-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Revisions by:
 * 		Pete Goodeve
 *		Philippe Houdoin
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
 
#include "PatchRow.h"

#include <stdio.h>

#include <Catalog.h>
#include <CheckBox.h>
#include <Debug.h>
#include <MidiRoster.h>
#include <MidiConsumer.h>
#include <MidiProducer.h>
#include <Window.h>

#include "MidiEventMeter.h"

extern const float ROW_LEFT = 50.0f;
extern const float ROW_TOP = 50.0f;
extern const float ROW_HEIGHT = 40.0f;
extern const float COLUMN_WIDTH = 40.0f;
extern const float METER_PADDING = 15.0f;
extern const uint32 MSG_CONNECT_REQUEST = 'mCRQ';

static const BPoint kBoxOffset(8, 7);


#define B_TRANSLATION_CONTEXT "PatchBay Rows"

static const char* kPatchCheckBoxTooltip = B_TRANSLATE(
	"%producer_name%\n"
	"  to\n"
	"%consumer_name%"
);


// PatchCheckBox is the check box that describes a connection
// between a producer and a consumer.
class PatchCheckBox : public BCheckBox
{
public:
	PatchCheckBox(BRect r, int32 producerID, int32 consumerID)
		:
		BCheckBox(r, "", "", new BMessage(MSG_CONNECT_REQUEST)),
		fProducerID(producerID),
		fConsumerID(consumerID)
	{
		BString tooltip;

		BMidiRoster* roster = BMidiRoster::MidiRoster();
		if (roster) {
			BMidiProducer* producer = roster->FindProducer(producerID);
			BMidiConsumer* consumer = roster->FindConsumer(consumerID);
			if (producer != NULL && consumer != NULL) {
				tooltip.SetTo(kPatchCheckBoxTooltip);
				tooltip.ReplaceAll("%producer_name%", producer->Name());
				tooltip.ReplaceAll("%consumer_name%", consumer->Name());
			}
			if (producer)
				producer->Release();
			if (consumer)
				consumer->Release();
		}

		if (tooltip.IsEmpty())
			tooltip << producerID << " -> " << consumerID;

		SetToolTip(tooltip);
	}

	int32 ProducerID() const
	{
		return fProducerID;
	}
	int32 ConsumerID() const
	{
		return fConsumerID;
	}

	void DoConnect();
	
private:
	int32 fProducerID;
	int32 fConsumerID;
};


PatchRow::PatchRow(int32 producerID)
	:
	BView(BRect(0, 0, 0, 0), "PatchRow", B_FOLLOW_NONE,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_PULSE_NEEDED),
	fProducerID(producerID),
	fEventMeter(NULL)
{
	fEventMeter = new MidiEventMeter(fProducerID);
}


PatchRow::~PatchRow()
{
	delete fEventMeter;
}


int32
PatchRow::ID() const
{
	return fProducerID;
}


void
PatchRow::Pulse()
{
	if (fEventMeter != NULL)
		fEventMeter->Pulse(this);
}


void
PatchRow::Draw(BRect)
{
	if (fEventMeter != NULL)
		fEventMeter->Draw(this);
}


void
PatchRow::AddColumn(int32 consumerID)
{
	BRect rect;
	int32 numColumns = CountChildren();
	rect.left = numColumns * COLUMN_WIDTH + METER_PADDING + kBoxOffset.x;
	rect.top = kBoxOffset.y;
	rect.right = rect.left + 20;
	rect.bottom = rect.top + 20;
	
	PatchCheckBox* box = new PatchCheckBox(rect, fProducerID, consumerID);
	AddChild(box);
	box->SetTarget(this);
}


void
PatchRow::RemoveColumn(int32 consumerID)
{
	int32 numChildren = CountChildren();
	for (int32 i = 0; i < numChildren; i++) {
		PatchCheckBox* box = dynamic_cast<PatchCheckBox*>(ChildAt(i));
		if (box != NULL && box->ConsumerID() == consumerID) {
			RemoveChild(box);
			delete box;
			while (i < numChildren) {
				box = dynamic_cast<PatchCheckBox*>(ChildAt(i++));
				if (box != NULL)
					box->MoveBy(-COLUMN_WIDTH, 0);
			}
			break;
		}
	}
}


void
PatchRow::Connect(int32 consumerID)
{
	int32 numChildren = CountChildren();
	for (int32 i = 0; i < numChildren; i++) {
		PatchCheckBox* box = dynamic_cast<PatchCheckBox*>(ChildAt(i));
		if (box != NULL && box->ConsumerID() == consumerID)
			box->SetValue(1);
	}
}


void
PatchRow::Disconnect(int32 consumerID)
{
	int32 numChildren = CountChildren();
	for (int32 i = 0; i < numChildren; i++) {
		PatchCheckBox* box = dynamic_cast<PatchCheckBox*>(ChildAt(i));
		if (box != NULL && box->ConsumerID() == consumerID)
			box->SetValue(0);
	}
}


void
PatchRow::AttachedToWindow()
{
	Window()->SetPulseRate(200000);
	AdoptParentColors();
	int32 numChildren = CountChildren();
	for (int32 i = 0; i < numChildren; i++) {
		PatchCheckBox* box = dynamic_cast<PatchCheckBox*>(ChildAt(i));
		if (box != NULL)
			box->SetTarget(this);
	}
}


void
PatchRow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case MSG_CONNECT_REQUEST:
		{
			BControl* ctrl;
			if (msg->FindPointer("source", (void**) &ctrl) == B_OK) {
				PatchCheckBox* box = dynamic_cast<PatchCheckBox*>(ctrl);
				if (box != NULL)
					box->DoConnect();
			}
		}
		break;
	default:
		BView::MessageReceived(msg);
		break;
	}
}


void
PatchCheckBox::DoConnect()
{
	int32 value = Value();
	int32 inverseValue = (value + 1) % 2;

	BMidiRoster* roster = BMidiRoster::MidiRoster();
	if (roster == NULL) {
		SetValue(inverseValue);
		return;
	}
	
	BMidiProducer* producer = roster->FindProducer(fProducerID);
	BMidiConsumer* consumer = roster->FindConsumer(fConsumerID);	
	if (producer != NULL && consumer != NULL) {
		status_t err;
		if (value != 0)
			err = producer->Connect(consumer);
		else
			err = producer->Disconnect(consumer);
		
		if (err != B_OK) {
			SetValue(inverseValue);
		}
	} else
		SetValue(inverseValue);

	if (producer != NULL)
		producer->Release();
	if (consumer != NULL)
		consumer->Release();
}
