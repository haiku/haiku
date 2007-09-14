/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mike Berg (inseculous)
 *		Julun <host.haiku@gmx.de>
 */
#ifndef ANALOG_CLOCK_H
#define ANALOG_CLOCK_H


#include <View.h>


class BBitmap;
class OffscreenClock;


class TAnalogClock : public BView {
	public:
						TAnalogClock(BRect frame, const char *name, 
							uint32 resizeMask, uint32 flags);
		virtual 		~TAnalogClock();

		virtual void 	AttachedToWindow();		
		virtual void 	Draw(BRect updateRect);
		virtual void 	MessageReceived(BMessage *message);

		void 			SetTime(int32 hour, int32 minute, int32 second);

	private:
		void 			_InitView(BRect frame);

		BBitmap 		*fBitmap;
		OffscreenClock	*fClock;
};

#endif	// ANALOG_CLOCK_H

