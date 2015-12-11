/* PatchView.cpp
 * -------------
 * Implements the main PatchBay view class.
 *
 * Copyright 2013, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Revisions by Pete Goodeve
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
 
#include "PatchView.h"

#include <Application.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <Debug.h>
#include <IconUtils.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <Messenger.h>
#include <MidiRoster.h>
#include <Window.h>

#include "EndpointInfo.h"
#include "PatchRow.h"
#include "UnknownDeviceIcons.h"


#define B_TRANSLATION_CONTEXT "Patch Bay"


PatchView::PatchView(BRect rect)
	:
	BView(rect, "PatchView", B_FOLLOW_ALL, B_WILL_DRAW),
	fUnknownDeviceIcon(NULL)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BRect iconRect(0, 0, LARGE_ICON_SIZE - 1, LARGE_ICON_SIZE - 1);
	fUnknownDeviceIcon = new BBitmap(iconRect, B_RGBA32);
	if (BIconUtils::GetVectorIcon(
			UnknownDevice::kVectorIcon,
			sizeof(UnknownDevice::kVectorIcon),
			fUnknownDeviceIcon)	== B_OK)
		return;
	delete fUnknownDeviceIcon;
}


PatchView::~PatchView()
{
	delete fUnknownDeviceIcon;
}


void
PatchView::AttachedToWindow()
{
	BMidiRoster* roster = BMidiRoster::MidiRoster();
	if (roster == NULL) {
		PRINT(("Couldn't get MIDI roster\n"));
		be_app->PostMessage(B_QUIT_REQUESTED);
		return;
	}
	
	BMessenger msgr(this);
	roster->StartWatching(&msgr);
	SetHighUIColor(B_PANEL_TEXT_COLOR);
}


void
PatchView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case B_MIDI_EVENT:
		HandleMidiEvent(msg);
		break;
	default:
		BView::MessageReceived(msg);
		break;
	}
}


bool
PatchView::GetToolTipAt(BPoint point, BToolTip** tip)
{
	bool found = false;
	int32 index = 0;
	endpoint_itor begin, end;
	int32 size = fConsumers.size();
	for (int32 i = 0; !found && i < size; i++) {
		BRect r = ColumnIconFrameAt(i);
		if (r.Contains(point)) {
			begin = fConsumers.begin();
			end = fConsumers.end();
			found = true;
			index = i;
		}
	}
	size = fProducers.size();
	for (int32 i = 0; !found && i < size; i++) {
		BRect r = RowIconFrameAt(i);
		if (r.Contains(point)) {
			begin = fProducers.begin();
			end = fProducers.end();
			found = true;
			index = i;
		}
	}
	
	if (!found)
		return false;

	endpoint_itor itor;
	for (itor = begin; itor != end; itor++, index--)
		if (index <= 0)
			break;
	
	if (itor == end)
		return false;

	BMidiRoster* roster = BMidiRoster::MidiRoster();
	if (roster == NULL)
		return false;
	BMidiEndpoint* obj = roster->FindEndpoint(itor->ID());
	if (obj == NULL)
		return false;

	BString str;
	str << "<" << obj->ID() << ">: " << obj->Name();
	obj->Release();

	SetToolTip(str.String());

	*tip = ToolTip();

	return true;
}


void
PatchView::Draw(BRect /* updateRect */)
{
	// draw producer icons
	SetDrawingMode(B_OP_OVER);
	int32 index = 0;
	for (list<EndpointInfo>::const_iterator i = fProducers.begin();
		i != fProducers.end(); i++) {
			const BBitmap* bitmap = (i->Icon()) ? i->Icon() : fUnknownDeviceIcon;
			DrawBitmapAsync(bitmap, RowIconFrameAt(index++).LeftTop());
	}

	// draw consumer icons
	int32 index2 = 0;
	for (list<EndpointInfo>::const_iterator i = fConsumers.begin();
		i != fConsumers.end(); i++) {
			const BBitmap* bitmap = (i->Icon()) ? i->Icon() : fUnknownDeviceIcon;
			DrawBitmapAsync(bitmap, ColumnIconFrameAt(index2++).LeftTop());
	}

	if (index == 0 && index2 == 0) {
		const char* message = B_TRANSLATE("No MIDI devices found!");
		float width = StringWidth(message);
		BRect rect = Bounds();

		rect.top = rect.top + rect.bottom / 2;
		rect.left = rect.left + rect.right / 2;
		rect.left -= width / 2;

		DrawString(message, rect.LeftTop());

		// Since the message is centered, we need to redraw the whole view in
		// this case.
		SetFlags(Flags() | B_FULL_UPDATE_ON_RESIZE);
	} else
		SetFlags(Flags() & ~B_FULL_UPDATE_ON_RESIZE);
}


BRect
PatchView::ColumnIconFrameAt(int32 index) const
{
	BRect rect;
	rect.left = ROW_LEFT + METER_PADDING + index * COLUMN_WIDTH;
	rect.top = 10;
	rect.right = rect.left + 31;
	rect.bottom = rect.top + 31;
	return rect;
}


BRect
PatchView::RowIconFrameAt(int32 index) const
{
	BRect rect;
	rect.left = 10;
	rect.top = ROW_TOP + index * ROW_HEIGHT;
	rect.right = rect.left + 31;
	rect.bottom = rect.top + 31;
	return rect;
}


void
PatchView::HandleMidiEvent(BMessage* msg)
{
	SET_DEBUG_ENABLED(true);
	
	int32 op;
	if (msg->FindInt32("be:op", &op) != B_OK) {
		PRINT(("PatchView::HandleMidiEvent: \"op\" field not found\n"));
		return;
	}

	switch (op) {
	case B_MIDI_REGISTERED:
		{
			int32 id;
			if (msg->FindInt32("be:id", &id) != B_OK) {
				PRINT(("PatchView::HandleMidiEvent: \"be:id\""
					" field not found in B_MIDI_REGISTERED event\n"));
				break;
			}
			
			const char* type;
			if (msg->FindString("be:type", &type) != B_OK) {
				PRINT(("PatchView::HandleMidiEvent: \"be:type\""
					" field not found in B_MIDI_REGISTERED event\n"));
				break;
			}
			
			PRINT(("MIDI Roster Event B_MIDI_REGISTERED: id=%" B_PRId32
					", type=%s\n", id, type));
			if (strcmp(type, "producer") == 0)
				AddProducer(id);
			else if (strcmp(type, "consumer") == 0)
				AddConsumer(id);
		}
		break;
	case B_MIDI_UNREGISTERED:
		{
			int32 id;
			if (msg->FindInt32("be:id", &id) != B_OK) {
				PRINT(("PatchView::HandleMidiEvent: \"be:id\""
					" field not found in B_MIDI_UNREGISTERED\n"));
				break;
			}
			
			const char* type;
			if (msg->FindString("be:type", &type) != B_OK) {
				PRINT(("PatchView::HandleMidiEvent: \"be:type\""
					" field not found in B_MIDI_UNREGISTERED\n"));
				break;
			}
			
			PRINT(("MIDI Roster Event B_MIDI_UNREGISTERED: id=%" B_PRId32
					", type=%s\n", id, type));
			if (strcmp(type, "producer") == 0)
				RemoveProducer(id);
			else if (strcmp(type, "consumer") == 0)
				RemoveConsumer(id);
		}
		break;
	case B_MIDI_CHANGED_PROPERTIES:
		{
			int32 id;
			if (msg->FindInt32("be:id", &id) != B_OK) {
				PRINT(("PatchView::HandleMidiEvent: \"be:id\""
					" field not found in B_MIDI_CHANGED_PROPERTIES\n"));
				break;
			}
			
			const char* type;
			if (msg->FindString("be:type", &type) != B_OK) {
				PRINT(("PatchView::HandleMidiEvent: \"be:type\""
					" field not found in B_MIDI_CHANGED_PROPERTIES\n"));
				break;
			}
			
			BMessage props;
			if (msg->FindMessage("be:properties", &props) != B_OK) {
				PRINT(("PatchView::HandleMidiEvent: \"be:properties\""
					" field not found in B_MIDI_CHANGED_PROPERTIES\n"));
				break;
			}
			
			PRINT(("MIDI Roster Event B_MIDI_CHANGED_PROPERTIES: id=%" B_PRId32
					", type=%s\n", id, type));
			if (strcmp(type, "producer") == 0)
				UpdateProducerProps(id, &props);
			else if (strcmp(type, "consumer") == 0)
				UpdateConsumerProps(id, &props);
			
		}
		break;
	case B_MIDI_CHANGED_NAME:
	case B_MIDI_CHANGED_LATENCY:
		// we don't care about these
		break;
	case B_MIDI_CONNECTED:
		{
			int32 prod;
			if (msg->FindInt32("be:producer", &prod) != B_OK) {
				PRINT(("PatchView::HandleMidiEvent: \"be:producer\""
					" field not found in B_MIDI_CONNECTED\n"));
				break;
			}
			
			int32 cons;
			if (msg->FindInt32("be:consumer", &cons) != B_OK) {
				PRINT(("PatchView::HandleMidiEvent: \"be:consumer\""
					" field not found in B_MIDI_CONNECTED\n"));
				break;
			}
			PRINT(("MIDI Roster Event B_MIDI_CONNECTED: producer=%" B_PRId32
					", consumer=%" B_PRId32 "\n", prod, cons));
			Connect(prod, cons);
		}
		break;
	case B_MIDI_DISCONNECTED:
		{
			int32 prod;
			if (msg->FindInt32("be:producer", &prod) != B_OK) {
				PRINT(("PatchView::HandleMidiEvent: \"be:producer\""
					" field not found in B_MIDI_DISCONNECTED\n"));
				break;
			}
			
			int32 cons;
			if (msg->FindInt32("be:consumer", &cons) != B_OK) {
				PRINT(("PatchView::HandleMidiEvent: \"be:consumer\""
					" field not found in B_MIDI_DISCONNECTED\n"));
				break;
			}
			PRINT(("MIDI Roster Event B_MIDI_DISCONNECTED: producer=%" B_PRId32
					", consumer=%" B_PRId32 "\n", prod, cons));
			Disconnect(prod, cons);
		}
		break;
	default:
		PRINT(("PatchView::HandleMidiEvent: unknown opcode %" B_PRId32 "\n",
			op));
		break;
	}
}


void
PatchView::AddProducer(int32 id)
{
	EndpointInfo info(id);
	fProducers.push_back(info);
	
	Window()->BeginViewTransaction();
	PatchRow* row = new PatchRow(id);
	fPatchRows.push_back(row);	
	BPoint p1 = CalcRowOrigin(fPatchRows.size() - 1);
	BPoint p2 = CalcRowSize();
	row->MoveTo(p1);
	row->ResizeTo(p2.x, p2.y);
	for (list<EndpointInfo>::const_iterator i = fConsumers.begin();
		i != fConsumers.end(); i++)
			row->AddColumn(i->ID());
	AddChild(row);
	Invalidate();
	Window()->EndViewTransaction();
}


void
PatchView::AddConsumer(int32 id)
{
	EndpointInfo info(id);
	fConsumers.push_back(info);
	
	Window()->BeginViewTransaction();
	BPoint newSize = CalcRowSize();
	for (row_itor i = fPatchRows.begin(); i != fPatchRows.end(); i++) {
		(*i)->AddColumn(id);
		(*i)->ResizeTo(newSize.x, newSize.y - 1);
	}
	Invalidate();
	Window()->EndViewTransaction();
}


void
PatchView::RemoveProducer(int32 id)
{
	for (endpoint_itor i = fProducers.begin(); i != fProducers.end(); i++) {
		if (i->ID() == id) {
			fProducers.erase(i);
			break;
		}
	}

	Window()->BeginViewTransaction();
	for (row_itor i = fPatchRows.begin(); i != fPatchRows.end(); i++) {
		if ((*i)->ID() == id) {
			PatchRow* row = *i;
			i = fPatchRows.erase(i);
			RemoveChild(row);
			delete row;
			float moveBy = -1 * CalcRowSize().y;
			while (i != fPatchRows.end()) {
				(*i++)->MoveBy(0, moveBy);
			}
			break;
		}
	}
	Invalidate();
	Window()->EndViewTransaction();
}


void
PatchView::RemoveConsumer(int32 id)
{
	Window()->BeginViewTransaction();
	for (endpoint_itor i = fConsumers.begin(); i != fConsumers.end(); i++) {
		if (i->ID() == id) {
			fConsumers.erase(i);
			break;
		}
	}

	BPoint newSize = CalcRowSize();
	for (row_itor i = fPatchRows.begin(); i != fPatchRows.end(); i++) {
		(*i)->RemoveColumn(id);
		(*i)->ResizeTo(newSize.x, newSize.y - 1);
	}
	Invalidate();
	Window()->EndViewTransaction();
}


void
PatchView::UpdateProducerProps(int32 id, const BMessage* props)
{
	for (endpoint_itor i = fProducers.begin(); i != fProducers.end(); i++) {
		if (i->ID() == id) {
			i->UpdateProperties(props);
			Invalidate();
			break;
		}
	}
}


void
PatchView::UpdateConsumerProps(int32 id, const BMessage* props)
{
	for (endpoint_itor i = fConsumers.begin(); i != fConsumers.end(); i++) {
		if (i->ID() == id) {
			i->UpdateProperties(props);
			Invalidate();
			break;
		}
	}
}


void
PatchView::Connect(int32 prod, int32 cons)
{
	for (row_itor i = fPatchRows.begin(); i != fPatchRows.end(); i++) {
		if ((*i)->ID() == prod) {
			(*i)->Connect(cons);
			break;
		}
	}
}


void
PatchView::Disconnect(int32 prod, int32 cons)
{
	for (row_itor i = fPatchRows.begin(); i != fPatchRows.end(); i++) {
		if ((*i)->ID() == prod) {
			(*i)->Disconnect(cons);
			break;
		}
	}
}


BPoint
PatchView::CalcRowOrigin(int32 rowIndex) const
{
	BPoint point;
	point.x = ROW_LEFT;
	point.y = ROW_TOP + rowIndex * ROW_HEIGHT;
	return point;
}


BPoint
PatchView::CalcRowSize() const
{
	BPoint point;
	point.x = METER_PADDING + fConsumers.size()*COLUMN_WIDTH;
	point.y = ROW_HEIGHT - 1;
	return point;
}
