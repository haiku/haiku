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


#include <OS.h>
#include <Locale.h>
#include <Messenger.h>
#include <View.h>

#include "PreferencesWindow.h" // For message constants


class BCountry;
class BMessageRunner;

#ifdef AS_REPLICANT
class _EXPORT	TTimeView;
#endif

class TTimeView : public BView {
public:
								TTimeView(float maxWidth, float height,
									bool use24HourClock, bool showSeconds,
									bool showDayOfWeek);
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

				bool			Orientation() const;
				void			SetOrientation(bool o);

				bool			Use24HourClock() const;
				void			SetUse24HourClock(bool use24HourClock);

				bool			ShowSeconds() const;
				void			SetShowSeconds(bool show);

				bool			ShowDayOfWeek() const;
				void			SetShowDayOfWeek(bool show);

				void			ShowCalendar(BPoint where);

private:
		friend class TReplicantTray;

				void			GetCurrentTime();
				void			GetCurrentDate();
				void			CalculateTextPlacement();
				void			ShowTimeOptions(BPoint);
				void			Update();
				void			UpdateTimeFormat();

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
				bool			fOrientation; // vertical = true

				bool			fUse24HourClock;
				bool			fShowSeconds;
				bool			fShowDayOfWeek;
				BString			fTimeFormat;

				BPoint			fTimeLocation;
				BPoint			fDateLocation;

				BMessenger		fCalendarWindow;

				// For date and time localization purposes
				BLocale			fLocale;
};


inline bool
TTimeView::Orientation() const
{
	return fOrientation;
}


#endif	/* TIME_VIEW_H */
