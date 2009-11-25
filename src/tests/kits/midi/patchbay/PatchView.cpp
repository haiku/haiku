// PatchView.cpp
// -------------
// Implements the main PatchBay view class.
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#include <Application.h>
#include <Bitmap.h>
#include <Debug.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <Messenger.h>
#include <MidiRoster.h>
#include <Window.h>
#include "EndpointInfo.h"
#include "PatchView.h"
#include "PatchRow.h"
#include "TToolTip.h"
#include "UnknownDeviceIcons.h"

PatchView::PatchView(BRect r)
	: BView(r, "PatchView", B_FOLLOW_ALL, B_WILL_DRAW),
	m_unknownDeviceIcon(NULL), m_trackIndex(-1), m_trackType(TRACK_COLUMN)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	uint8 iconSize;
	const uint8* iconData;
	
	if (DISPLAY_ICON_SIZE == B_LARGE_ICON) {
		iconSize = LARGE_ICON_SIZE;
		iconData = UnknownDevice::kLargeIconBits;
	} else {
		iconSize = MINI_ICON_SIZE;
		iconData = UnknownDevice::kMiniIconBits;
	}
	
	BRect iconFrame(0, 0, iconSize-1, iconSize-1);
	m_unknownDeviceIcon = new BBitmap(iconFrame, ICON_COLOR_SPACE);
	memcpy(m_unknownDeviceIcon->Bits(), iconData, m_unknownDeviceIcon->BitsLength());	
}

PatchView::~PatchView()
{
	delete m_unknownDeviceIcon;
}

void PatchView::AttachedToWindow()
{
	BMidiRoster* roster = BMidiRoster::MidiRoster();
	if (! roster) {
		PRINT(("Couldn't get MIDI roster\n"));
		be_app->PostMessage(B_QUIT_REQUESTED);
		return;
	}
	
	BMessenger msgr(this);
	roster->StartWatching(&msgr);
}

void PatchView::MessageReceived(BMessage* msg)
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

void PatchView::MouseMoved(BPoint pt, uint32 transit, const BMessage* /* dragMsg */)
{
	if (transit == B_EXITED_VIEW || transit == B_OUTSIDE_VIEW) {
		StopTipTracking();
		return;
	}	
	
	bool found = false;
	int32 size = m_consumers.size();
	for (int32 i=0; (! found) && i<size; i++) {
		BRect r = ColumnIconFrameAt(i);
		if (r.Contains(pt)) {
			StartTipTracking(pt, r, i, TRACK_COLUMN);
			found = true;
		}
	}
	size = m_producers.size();
	for (int32 i=0; (! found) && i<size; i++) {
		BRect r = RowIconFrameAt(i);
		if (r.Contains(pt)) {
			StartTipTracking(pt, r, i, TRACK_ROW);
			found = true;
		}
	}
}

void PatchView::StopTipTracking()
{
	m_trackIndex = -1;
	m_trackType = TRACK_COLUMN;
	StopTip();
}

void PatchView::StartTipTracking(BPoint pt, BRect rect, int32 index, track_type type)
{	
	if (index == m_trackIndex && type == m_trackType)
		return;
		
	StopTip();
	m_trackIndex = index;
	m_trackType = type;
	
	endpoint_itor begin, end;
	if (type == TRACK_COLUMN) {
		begin = m_consumers.begin();
		end = m_consumers.end();
	} else {
		begin = m_producers.begin();
		end = m_producers.end();
	}
	
	endpoint_itor i;
	for (i = begin; i != end; i++, index--)
		if (index <= 0) break;
	
	if (i == end)
		return;
			
	BMidiRoster* roster = BMidiRoster::MidiRoster();
	if (! roster)
		return;
	BMidiEndpoint* obj = roster->FindEndpoint(i->ID());
	if (! obj)
		return;

	BString str;
	str << "<" << obj->ID() << ">: " << obj->Name();
	obj->Release();
	StartTip(pt, rect, str.String());
}

void PatchView::StartTip(BPoint pt, BRect rect, const char* str)
{
	BMessage msg(eToolTipStart);
	msg.AddPoint("start", ConvertToScreen(pt));
	msg.AddRect("bounds", ConvertToScreen(rect));
	msg.AddString("string", str);
	be_app->PostMessage(&msg);
}

void PatchView::StopTip()
{
	be_app->PostMessage(eToolTipStop);
}

void PatchView::Draw(BRect /* updateRect */)
{
	// draw producer icons
	SetDrawingMode(B_OP_OVER);
	int32 index = 0;
	for (list<EndpointInfo>::const_iterator i = m_producers.begin(); i != m_producers.end(); i++) {
		const BBitmap* bitmap = (i->Icon()) ? i->Icon() : m_unknownDeviceIcon;
		DrawBitmapAsync(bitmap, RowIconFrameAt(index++).LeftTop());
	}
			
	// draw consumer icons
	index = 0;
	for (list<EndpointInfo>::const_iterator i = m_consumers.begin(); i != m_consumers.end(); i++) {
		const BBitmap* bitmap = (i->Icon()) ? i->Icon() : m_unknownDeviceIcon;
		DrawBitmapAsync(bitmap, ColumnIconFrameAt(index++).LeftTop());
	}
}

BRect PatchView::ColumnIconFrameAt(int32 index) const
{
	BRect r;
	r.left = ROW_LEFT + METER_PADDING + index*COLUMN_WIDTH;
	r.top = 10;
	r.right = r.left + 31;
	r.bottom = r.top + 31;
	return r;
}

BRect PatchView::RowIconFrameAt(int32 index) const
{
	BRect r;
	r.left = 10;
	r.top = ROW_TOP + index*ROW_HEIGHT;
	r.right = r.left + 31;
	r.bottom = r.top + 31;
	return r;
}

void PatchView::HandleMidiEvent(BMessage* msg)
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
				PRINT(("PatchView::HandleMidiEvent: \"be:id\" field not found in B_MIDI_REGISTERED event\n"));
				break;
			}
			
			const char* type;
			if (msg->FindString("be:type", &type) != B_OK) {
				PRINT(("PatchView::HandleMidiEvent: \"be:type\" field not found in B_MIDI_REGISTERED event\n"));
				break;
			}
			
			PRINT(("MIDI Roster Event B_MIDI_REGISTERED: id=%ld, type=%s\n", id, type));
			if (! strcmp(type, "producer")) {
				AddProducer(id);
			} else if (! strcmp(type, "consumer")) {
				AddConsumer(id);
			}
		}
		break;
	case B_MIDI_UNREGISTERED:
		{
			int32 id;
			if (msg->FindInt32("be:id", &id) != B_OK) {
				PRINT(("PatchView::HandleMidiEvent: \"be:id\" field not found in B_MIDI_UNREGISTERED\n"));
				break;
			}
			
			const char* type;
			if (msg->FindString("be:type", &type) != B_OK) {
				PRINT(("PatchView::HandleMidiEvent: \"be:type\" field not found in B_MIDI_UNREGISTERED\n"));
				break;
			}
			
			PRINT(("MIDI Roster Event B_MIDI_UNREGISTERED: id=%ld, type=%s\n", id, type));
			if (! strcmp(type, "producer")) {
				RemoveProducer(id);
			} else if (! strcmp(type, "consumer")) {
				RemoveConsumer(id);
			}
		}
		break;
	case B_MIDI_CHANGED_PROPERTIES:
		{
			int32 id;
			if (msg->FindInt32("be:id", &id) != B_OK) {
				PRINT(("PatchView::HandleMidiEvent: \"be:id\" field not found in B_MIDI_CHANGED_PROPERTIES\n"));
				break;
			}
			
			const char* type;
			if (msg->FindString("be:type", &type) != B_OK) {
				PRINT(("PatchView::HandleMidiEvent: \"be:type\" field not found in B_MIDI_CHANGED_PROPERTIES\n"));
				break;
			}
			
			BMessage props;
			if (msg->FindMessage("be:properties", &props) != B_OK) {
				PRINT(("PatchView::HandleMidiEvent: \"be:properties\" field not found in B_MIDI_CHANGED_PROPERTIES\n"));
				break;
			}
			
			PRINT(("MIDI Roster Event B_MIDI_CHANGED_PROPERTIES: id=%ld, type=%s\n", id, type));
			if (! strcmp(type, "producer")) {
				UpdateProducerProps(id, &props);
			} else if (! strcmp(type, "consumer")) {
				UpdateConsumerProps(id, &props);
			}
			
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
				PRINT(("PatchView::HandleMidiEvent: \"be:producer\" field not found in B_MIDI_CONNECTED\n"));
				break;
			}
			
			int32 cons;
			if (msg->FindInt32("be:consumer", &cons) != B_OK) {
				PRINT(("PatchView::HandleMidiEvent: \"be:consumer\" field not found in B_MIDI_CONNECTED\n"));
				break;
			}
			PRINT(("MIDI Roster Event B_MIDI_CONNECTED: producer=%ld, consumer=%ld\n", prod, cons));
			Connect(prod, cons);
		}
		break;
	case B_MIDI_DISCONNECTED:
		{
			int32 prod;
			if (msg->FindInt32("be:producer", &prod) != B_OK) {
				PRINT(("PatchView::HandleMidiEvent: \"be:producer\" field not found in B_MIDI_DISCONNECTED\n"));
				break;
			}
			
			int32 cons;
			if (msg->FindInt32("be:consumer", &cons) != B_OK) {
				PRINT(("PatchView::HandleMidiEvent: \"be:consumer\" field not found in B_MIDI_DISCONNECTED\n"));
				break;
			}
			PRINT(("MIDI Roster Event B_MIDI_DISCONNECTED: producer=%ld, consumer=%ld\n", prod, cons));
			Disconnect(prod, cons);
		}
		break;
	default:
		PRINT(("PatchView::HandleMidiEvent: unknown opcode %ld\n", op));
		break;
	}
}

void PatchView::AddProducer(int32 id)
{
	EndpointInfo info(id);
	m_producers.push_back(info);
	
	Window()->BeginViewTransaction();
	PatchRow* row = new PatchRow(id);
	m_patchRows.push_back(row);	
	BPoint p1 = CalcRowOrigin(m_patchRows.size() - 1);
	BPoint p2 = CalcRowSize();
	row->MoveTo(p1);
	row->ResizeTo(p2.x, p2.y);
	for (list<EndpointInfo>::const_iterator i = m_consumers.begin(); i != m_consumers.end(); i++) {
		row->AddColumn(i->ID());
	}
	AddChild(row);
	Invalidate();
	Window()->EndViewTransaction();
}

void PatchView::AddConsumer(int32 id)
{
	EndpointInfo info(id);
	m_consumers.push_back(info);
	
	Window()->BeginViewTransaction();
	BPoint newSize = CalcRowSize();
	for (row_itor i = m_patchRows.begin(); i != m_patchRows.end(); i++) {
		(*i)->AddColumn(id);
		(*i)->ResizeTo(newSize.x, newSize.y - 1);
	}
	Invalidate();
	Window()->EndViewTransaction();
}

void PatchView::RemoveProducer(int32 id)
{
	for (endpoint_itor i = m_producers.begin(); i != m_producers.end(); i++) {
		if (i->ID() == id) {
			m_producers.erase(i);
			break;
		}
	}

	Window()->BeginViewTransaction();
	for (row_itor i = m_patchRows.begin(); i != m_patchRows.end(); i++) {
		if ((*i)->ID() == id) {
			PatchRow* row = *i;
			i = m_patchRows.erase(i);
			RemoveChild(row);
			delete row;
			float moveBy = -1*CalcRowSize().y;
			while (i != m_patchRows.end()) {
				(*i++)->MoveBy(0, moveBy);
			}
			break;
		}
	}
	Invalidate();
	Window()->EndViewTransaction();
}

void PatchView::RemoveConsumer(int32 id)
{
	Window()->BeginViewTransaction();
	for (endpoint_itor i = m_consumers.begin(); i != m_consumers.end(); i++) {
		if (i->ID() == id) {
			m_consumers.erase(i);
			break;
		}
	}

	BPoint newSize = CalcRowSize();
	for (row_itor i = m_patchRows.begin(); i != m_patchRows.end(); i++) {
		(*i)->RemoveColumn(id);
		(*i)->ResizeTo(newSize.x, newSize.y - 1);
	}
	Invalidate();
	Window()->EndViewTransaction();
}

void PatchView::UpdateProducerProps(int32 id, const BMessage* props)
{
	for (endpoint_itor i = m_producers.begin(); i != m_producers.end(); i++) {
		if (i->ID() == id) {
			i->UpdateProperties(props);
			Invalidate();
			break;
		}
	}
}

void PatchView::UpdateConsumerProps(int32 id, const BMessage* props)
{
	for (endpoint_itor i = m_consumers.begin(); i != m_consumers.end(); i++) {
		if (i->ID() == id) {
			i->UpdateProperties(props);
			Invalidate();
			break;
		}
	}
}

void PatchView::Connect(int32 prod, int32 cons)
{
	for (row_itor i = m_patchRows.begin(); i != m_patchRows.end(); i++) {
		if ((*i)->ID() == prod) {
			(*i)->Connect(cons);
			break;
		}
	}
}

void PatchView::Disconnect(int32 prod, int32 cons)
{
	for (row_itor i = m_patchRows.begin(); i != m_patchRows.end(); i++) {
		if ((*i)->ID() == prod) {
			(*i)->Disconnect(cons);
			break;
		}
	}
}

BPoint PatchView::CalcRowOrigin(int32 rowIndex) const
{
	BPoint pt;
	pt.x = ROW_LEFT;
	pt.y = ROW_TOP + rowIndex*ROW_HEIGHT;
	return pt;
}

BPoint PatchView::CalcRowSize() const
{
	BPoint pt;
	pt.x = METER_PADDING + m_consumers.size()*COLUMN_WIDTH;
	pt.y = ROW_HEIGHT - 1;
	return pt;
}
