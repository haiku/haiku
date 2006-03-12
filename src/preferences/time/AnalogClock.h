/*
 * Copyright 2004-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mike Berg (inseculous)
 */
#ifndef ANALOG_CLOCK_H
#define ANALOG_CLOCK_H

#include <Message.h>
#include <View.h>

class OffscreenClock;

class TAnalogClock: public BView {
	public:
		TAnalogClock(BRect frame, const char *name, uint32 resizingmode, uint32 flags);
		virtual ~TAnalogClock();

		virtual void AttachedToWindow();		
		virtual void Draw(BRect updaterect);
		virtual void MessageReceived(BMessage *);

		void SetTo(int32 hour, int32 minute, int32 second);

	private:
		void _InitView(BRect frame);

		BPoint fClockLeftTop;
		BBitmap *fBitmap;
		OffscreenClock *fClock;
};

#endif	// ANALOG_CLOCK_H
