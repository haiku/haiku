/*
 * Copyright 2004-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew McCall <mccall@@digitalparadise.co.uk>
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 *		Philippe Saint-Pierre <stpere@gmail.com>
 *		Hamish Morrison <hamish@lavabit.com>
 */
#ifndef _DATE_TIME_VIEW_H
#define _DATE_TIME_VIEW_H


#include <LayoutBuilder.h>


class TDateEdit;
class TTimeEdit;
class TAnalogClock;


namespace BPrivate {
	class BCalendarView;
}
using BPrivate::BCalendarView;


class DateTimeView : public BGroupView {
public:
								DateTimeView(const char* name);
	virtual 					~DateTimeView();

	virtual	void			 	AttachedToWindow();
	virtual	void 				MessageReceived(BMessage* message);

			bool				CheckCanRevert();
			bool				GetUseGmtTime();

private:
			void 				_InitView();
			void 				_UpdateDateTime(BMessage* message);
			void 				_NotifyClockSettingChanged();
			void				_Revert();
			time_t				_PrefletUptime() const;

			TDateEdit*			fDateEdit;
			TTimeEdit*			fTimeEdit;
			BCalendarView*		fCalendarView;
			TAnalogClock*		fClock;

			bool				fInitialized;

			time_t				fTimeAtStart;
			bigtime_t			fSystemTimeAtStart;
};


#endif	// _DATE_TIME_VIEW_H
