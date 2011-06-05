/*
 * Copyright 2004-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 *		Hamish Morrison <hamish@lavabit.com>
 */
#ifndef _ANALOG_CLOCK_H
#define _ANALOG_CLOCK_H


#include <View.h>


class TAnalogClock : public BView {
public:
							TAnalogClock(const char* name,
								bool drawSecondHand = true,
								bool interactive = true);
	virtual					~TAnalogClock();

	virtual	void			Draw(BRect updateRect);
	virtual	void			MessageReceived(BMessage* message);
	virtual	void			MouseDown(BPoint point);
	virtual	void			MouseUp(BPoint point);
	virtual	void			MouseMoved(BPoint point, uint32 transit,
								const BMessage* message);
	virtual	void			FrameResized(float, float);
	
	virtual	BSize			MaxSize();
	virtual	BSize			MinSize();
	virtual	BSize			PreferredSize();

			void			SetTime(int32 hour, int32 minute, int32 second);
			bool			IsChangingTime();
			void			ChangeTimeFinished();

			void 			GetTime(int32* hour, int32* minute, int32* second);
			void 			DrawClock();

			bool			InHourHand(BPoint point);
			bool			InMinuteHand(BPoint point);

			void			SetHourHand(BPoint point);
			void			SetMinuteHand(BPoint point);

			void			SetHourDragging(bool dragging);
			void			SetMinuteDragging(bool dragging);
private:
			float			_GetPhi(BPoint point);
			bool			_InHand(BPoint point, int32 ticks, float radius);
			void			_DrawHands(float x, float y, float radius,
								rgb_color hourHourColor,
								rgb_color hourMinuteColor,
								rgb_color secondsColor, rgb_color knobColor);

			int32			fHours;
			int32			fMinutes;
			int32			fSeconds;
			bool			fDirty;
			
			float 			fCenterX;
			float			fCenterY;
			float			fRadius;

			bool			fHourDragging;
			bool			fMinuteDragging;
			bool			fDrawSecondHand;
			bool			fInteractive;

			bool			fTimeChangeIsOngoing;	
};


#endif	// _ANALOG_CLOCK_H
