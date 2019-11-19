/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered
trademarks of Be Incorporated in the United States and other countries. Other
brand product names are registered trademarks or trademarks of their respective
holders.
All rights reserved.
*/
#ifndef TIME_VIEW_H
#define TIME_VIEW_H


#include <DateFormat.h>
#include <DateTimeFormat.h>
#include <Locale.h>
#include <Messenger.h>
#include <OS.h>
#include <View.h>


// open Time preferences
const uint32 kChangeTime = 'ChTm';

// pop the calendar
const uint32 kShowCalendar = 'ShCa';

// show or hide clock
const uint32 kShowHideTime = 'ShTm';

// show seconds
const uint32 kShowSeconds = 'SwSc';

// show day of week
const uint32 kShowDayOfWeek = 'SwDw';

// show time zone
const uint32 kShowTimeZone = 'SwTz';

// get clock settings to send to Time prefs
const uint32 kGetClockSettings = 'GCkS';


class BCountry;
class BMessageRunner;
class CalendarMenuWindow;
class TBarView;

#ifdef AS_REPLICANT
class _EXPORT	TTimeView;
#endif

class TTimeView : public BView {
public:
								TTimeView(float maxWidth, float height,
									TBarView* barView = NULL);
								TTimeView(BMessage* data);
								~TTimeView();

#ifdef AS_REPLICANT
				status_t		Archive(BMessage* data,
									bool deep = true) const;
		static	BArchivable*	Instantiate(BMessage* data);
#endif

				void			AttachedToWindow();
				void			Draw(BRect update);
				void			FrameMoved(BPoint);
				void			GetPreferredSize(float* width, float* height);
				void			MessageReceived(BMessage*);
				void			MouseDown(BPoint where);
				void			Pulse();
				void			ResizeToPreferred();

				bool			ShowSeconds() const;
				void			SetShowSeconds(bool show);

				bool			ShowDayOfWeek() const;
				void			SetShowDayOfWeek(bool show);

				bool			ShowTimeZone() const;
				void			SetShowTimeZone(bool show);

				void			ShowCalendar(BPoint where);
				bool			IsShowingCalendar();

private:
		friend class TReplicantTray;

				void			GetCurrentTime();
				void			GetCurrentDate();
				void			UpdateTimeFormat();
				void			CalculateTextPlacement();
				void			ShowTimeOptions(BPoint);
				void			Update();

				bool			Vertical();

				TBarView*		fBarView;
				BView*			fParent;
				bool			fNeedToUpdate;

				time_t			fCurrentTime;
				time_t			fLastTime;

				char			fCurrentTimeStr[64];
				char			fLastTimeStr[64];
				char			fCurrentDateStr[64];
				char			fLastDateStr[64];

				int				fSeconds;
				int				fMinute;
				int				fHour;

				float			fMaxWidth;
				float			fHeight;

				int16			fShowLevel;

				bool			fShowSeconds;
				bool			fShowDayOfWeek;
				bool			fShowTimeZone;

				BPoint			fTimeLocation;
				BPoint			fDateLocation;

				BMessenger		fCalendarWindowMessenger;
		CalendarMenuWindow* 	fCalendarWindow;

				// For date and time localization purposes
				BDateTimeFormat* fTimeFormat;
				BDateFormat*	fDateFormat;
};


#endif	/* TIME_VIEW_H */
