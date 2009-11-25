// PatchView.h
// -----------
// The main PatchBay view contains a row of icons along the top and
// left sides representing available consumers and producers, and
// a set of PatchRows which build the matrix of connections.
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#ifndef _PatchView_h
#define _PatchView_h

#include <Rect.h>
#include <View.h>
#include <list>
#include "EndpointInfo.h"

class PatchRow;
class BBitmap;
class TToolTip;

using namespace std;

class PatchView : public BView
{
public:
	PatchView(BRect r);
	~PatchView();
	
	void AttachedToWindow();
	void MessageReceived(BMessage* msg);
	void Draw(BRect updateRect);
	void MouseMoved(BPoint point, uint32 transit, const BMessage* dragMsg);
	
private:
	typedef enum {	
		TRACK_COLUMN,
		TRACK_ROW
	} track_type;
	
	BRect ColumnIconFrameAt(int32 index) const;
	BRect RowIconFrameAt(int32 index) const;
	void StartTipTracking(BPoint pt, BRect rect, int32 index, track_type type=TRACK_COLUMN);
	void StopTipTracking();
	void StartTip(BPoint pt, BRect rect, const char* str);
	void StopTip();
	
	void AddProducer(int32 id);
	void AddConsumer(int32 id);
	void RemoveProducer(int32 id);
	void RemoveConsumer(int32 id);
	void UpdateProducerProps(int32 id, const BMessage* props);
	void UpdateConsumerProps(int32 id, const BMessage* props);
	void Connect(int32 prod, int32 cons);
	void Disconnect(int32 prod, int32 cons);
	
	void HandleMidiEvent(BMessage* msg);
	
	BPoint CalcRowOrigin(int32 rowIndex) const;
	BPoint CalcRowSize() const;
	
	typedef list<EndpointInfo>::iterator endpoint_itor;
	typedef list<EndpointInfo>::const_iterator const_endpoint_itor;
	typedef list<PatchRow*>::iterator row_itor;
	
	list<EndpointInfo> m_producers;
	list<EndpointInfo> m_consumers;
	list<PatchRow*> m_patchRows;
	BBitmap* m_unknownDeviceIcon;
	int32 m_trackIndex;
	track_type m_trackType;
	TToolTip* m_toolTip;
};

#endif /* _PatchView_h */
