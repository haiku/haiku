/* PatchView.h
 * -----------
 * The main PatchBay view contains a row of icons along the top and
 * left sides representing available consumers and producers, and
 * a set of PatchRows which build the matrix of connections.
 *
 * Copyright 2013, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Revisions by Pete Goodeve
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
#ifndef PATCHVIEW_H
#define PATCHVIEW_H

#include <Rect.h>
#include <View.h>
#include <list>

#include "EndpointInfo.h"

class PatchRow;
class BBitmap;

using namespace std;

class PatchView : public BView
{
public:
	PatchView(BRect r);
	~PatchView();
	
	void AttachedToWindow();
	void MessageReceived(BMessage* msg);
	void Draw(BRect updateRect);
	
private:
	typedef enum {	
		TRACK_COLUMN,
		TRACK_ROW
	} track_type;
	
	BRect ColumnIconFrameAt(int32 index) const;
	BRect RowIconFrameAt(int32 index) const;
	virtual bool GetToolTipAt(BPoint point, BToolTip** tip);

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
	
	list<EndpointInfo> fProducers;
	list<EndpointInfo> fConsumers;
	list<PatchRow*> fPatchRows;
	BBitmap* fUnknownDeviceIcon;
};

#endif /* PATCHVIEW_H */
