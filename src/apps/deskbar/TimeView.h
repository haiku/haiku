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

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/
#ifndef TIME_VIEW_H
#define TIME_VIEW_H


#include <OS.h>
#include <View.h>


const uint32 kMsgShowSeconds = 'ShSc';
const uint32 kMsgMilTime = 'MilT';
const uint32 kMsgFullDate = 'FDat';
const uint32 kMsgEuroDate = 'EDat';


#ifdef AS_REPLICANT
class _EXPORT	TTimeView;
#endif

class TTimeView : public BView {
	public:
		TTimeView(bool showSeconds = false, bool milTime = false, bool fullDate = false,
			bool euroDate = false, bool showInterval = false);
		TTimeView(BMessage *data);
		~TTimeView();

#ifdef AS_REPLICANT
		status_t Archive(BMessage *data, bool deep = true) const;
		static BArchivable *Instantiate(BMessage *data);
#endif

		void		AttachedToWindow();
		void		Draw(BRect update);
		void		GetPreferredSize(float *width, float *height);
		void		ResizeToPreferred();
		void		FrameMoved(BPoint);
		void		MessageReceived(BMessage*);
		void		MouseDown(BPoint where);
		void		Pulse();

		bool		ShowingSeconds() 	{ return fShowSeconds; }
		void		ShowSeconds(bool);
		bool		ShowingMilTime()	{ return fMilTime; }
		void		ShowMilTime(bool);
		bool		ShowingDate()		{ return fShowingDate; }
		void		ShowDate(bool);
		bool		ShowingFullDate()	{ return fFullDate; }
		void		ShowFullDate(bool);
		bool		CanShowFullDate() const { return fCanShowFullDate; }
		void		AllowFullDate(bool);
		bool		ShowingEuroDate()	{return fEuroDate; }
		void		ShowEuroDate(bool);

		bool		Orientation() const;
		void		SetOrientation(bool o);

	private:
		friend class TReplicantTray;

		void		Update();
		void		GetCurrentTime();
		void		GetCurrentDate();
		void		CalculateTextPlacement();
		void		ShowClockOptions(BPoint);

		BView		*fParent;
		bool		fNeedToUpdate;

		time_t		fTime;
		time_t		fLastTime;

		char		fTimeStr[64];
		char		fLastTimeStr[64];
		char		fDateStr[64];
		char		fLastDateStr[64];

		int			fSeconds;
		int			fMinute;
		int			fHour;
		bool		fInterval;

		bool		fShowInterval;
		bool		fShowSeconds;
		bool		fMilTime;
		bool		fShowingDate;
		bool		fFullDate;
		bool		fCanShowFullDate;
		bool		fEuroDate;

		float		fFontHeight;
		bool		fOrientation;		// vertical = true
		BPoint		fTimeLocation;
		BPoint		fDateLocation;
};


inline bool
TTimeView::Orientation() const
{
	return fOrientation;
}


#endif	/* TIME_VIEW_H */
