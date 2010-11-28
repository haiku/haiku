/*
 * Copyright 2004-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 */
#ifndef _ANALOG_CLOCK_H
#define _ANALOG_CLOCK_H


#include <View.h>


class BBitmap;
class OffscreenClock;


class TAnalogClock : public BView {
public:
								TAnalogClock(BRect frame, const char* name,
									bool drawSecondHand = true, bool interactive = true);
	virtual						~TAnalogClock();

	virtual	void				AttachedToWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				MouseDown(BPoint point);
	virtual	void				MouseUp(BPoint point);
	virtual	void				MouseMoved(BPoint point, uint32 transit,
									const BMessage* message);

			void				SetTime(int32 hour, int32 minute, int32 second);

			bool				IsChangingTime()
									{	return fTimeChangeIsOngoing;	}
			void				ChangeTimeFinished();
private:
			void				_InitView(BRect frame);

			BBitmap*			fBitmap;
			OffscreenClock*		fClock;

			bool				fDrawSecondHand;
			bool				fInteractive;
			bool				fDraggingHourHand;
			bool				fDraggingMinuteHand;

			bool				fTimeChangeIsOngoing;
};


#endif	// _ANALOG_CLOCK_H
