/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 */
#ifndef ANALOG_CLOCK_H
#define ANALOG_CLOCK_H


#include <View.h>


class BBitmap;
class OffscreenClock;


class TAnalogClock : public BView {
	public:
						TAnalogClock(BRect frame, const char *name);
		virtual 		~TAnalogClock();

		virtual void 	AttachedToWindow();		
		virtual void 	Draw(BRect updateRect);
		virtual void 	MessageReceived(BMessage *message);
		virtual void	MouseDown(BPoint point);
		virtual void	MouseUp(BPoint point);
		virtual void	MouseMoved(BPoint point, uint32 transit, const BMessage *message);

		void 			SetTime(int32 hour, int32 minute, int32 second);

		bool			IsChangingTime() {	return fTimeChangeIsOngoing;	}
		void			ChangeTimeFinished();
	private:
		void 			_InitView(BRect frame);

		BBitmap 		*fBitmap;
		OffscreenClock	*fClock;

		bool			fDraggingHourHand;
		bool			fDraggingMinuteHand;

		bool			fTimeChangeIsOngoing;
};

#endif	// ANALOG_CLOCK_H

