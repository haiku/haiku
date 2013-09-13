/* PatchRow.h
 * ----------
 *
 * Copyright 2013, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Revisions by Pete Goodeve
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
#ifndef PATCHROW_H
#define PATCHROW_H

#include <View.h>

extern const float ROW_LEFT;
extern const float ROW_TOP;
extern const float ROW_HEIGHT;
extern const float COLUMN_WIDTH;
extern const float METER_PADDING;
extern const uint32 MSG_CONNECT_REQUEST;

class MidiEventMeter;

class PatchRow : public BView
{
public:
	PatchRow(int32 producerID);
	~PatchRow();
	
	int32 ID() const;
	
	void AttachedToWindow();
	void MessageReceived(BMessage* msg);
	void Pulse();
	void Draw(BRect updateRect);
	
	void AddColumn(int32 consumerID);
	void RemoveColumn(int32 consumerID);
	void Connect(int32 consumerID);
	void Disconnect(int32 consumerID);

private:
	int32 fProducerID;
	MidiEventMeter* fEventMeter;
};

#endif /* PATCHROW_H */
