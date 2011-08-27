/*
 * Copyright 2007-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CALENDAR_VIEW_H_
#define _CALENDAR_VIEW_H_


#include "DateTime.h"


#include <Invoker.h>
#include <List.h>
#include <String.h>
#include <View.h>


class BMessage;


namespace BPrivate {


enum week_start {
	B_WEEK_START_MONDAY,
	B_WEEK_START_SUNDAY
};


class BCalendarView : public BView, public BInvoker {
	public:
								BCalendarView(BRect frame, const char *name,
									uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
									uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);

								BCalendarView(BRect frame, const char *name, week_start start,
									uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
									uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);

								BCalendarView(const char* name,
									uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);

								BCalendarView(const char* name, week_start start,
									uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);

		virtual					~BCalendarView();

								BCalendarView(BMessage *archive);
		static BArchivable*		Instantiate(BMessage *archive);
		virtual status_t		Archive(BMessage *archive, bool deep = true) const;

		virtual void			AttachedToWindow();
		virtual	void			DetachedFromWindow();

		virtual void			AllAttached();
		virtual void			AllDetached();

		virtual void			FrameMoved(BPoint newPosition);
		virtual void			FrameResized(float width, float height);

		virtual void			Draw(BRect updateRect);

		virtual void			DrawDay(BView *owner, BRect frame, const char *text,
									bool isSelected = false, bool isEnabled = true,
									bool focus = false);
		virtual void			DrawDayName(BView *owner, BRect frame, const char *text);
		virtual void			DrawWeekNumber(BView *owner, BRect frame, const char *text);

		virtual void			MessageReceived(BMessage *message);

		uint32					SelectionCommand() const;
		BMessage*				SelectionMessage() const;
		virtual void			SetSelectionMessage(BMessage *message);

		uint32					InvocationCommand() const;
		BMessage*				InvocationMessage() const;
		virtual void			SetInvocationMessage(BMessage *message);

		virtual void			WindowActivated(bool state);
		virtual void			MakeFocus(bool state = true);
		virtual status_t		Invoke(BMessage* message = NULL);

		virtual void			MouseUp(BPoint point);
		virtual void			MouseDown(BPoint where);
		virtual void			MouseMoved(BPoint point, uint32 code,
									const BMessage *dragMessage);

		virtual void			KeyDown(const char *bytes, int32 numBytes);

		virtual BHandler*		ResolveSpecifier(BMessage *message, int32 index,
									BMessage *specifier, int32 form, const char *property);
		virtual status_t		GetSupportedSuites(BMessage *data);
		virtual status_t		Perform(perform_code code, void* arg);

		virtual void			ResizeToPreferred();
		virtual void			GetPreferredSize(float *width, float *height);

		virtual	BSize			MaxSize();
		virtual	BSize			MinSize();
		virtual	BSize			PreferredSize();

		int32					Day() const;
		int32					Year() const;
		int32					Month() const;

		BDate					Date() const;
		bool					SetDate(const BDate &date);
		bool					SetDate(int32 year, int32 month, int32 day);

		week_start				WeekStart() const;
		void					SetWeekStart(week_start start);

		bool					IsDayNameHeaderVisible() const;
		void					SetDayNameHeaderVisible(bool visible);

		bool					IsWeekNumberHeaderVisible() const;
		void					SetWeekNumberHeaderVisible(bool visible);

	private:
		void					_InitObject();

		void					_SetToDay();
		void					_GetYearMonth(int32 *year, int32 *month) const;
		void					_GetPreferredSize(float *width, float *height);

		void					_SetupDayNames();
		void					_SetupDayNumbers();
		void					_SetupWeekNumbers();

		void					_DrawDays();
		void					_DrawFocusRect();
		void					_DrawDayHeader();
		void					_DrawWeekHeader();
		void					_DrawDay(int32 curRow, int32 curColumn,
									int32 row, int32 column, int32 counter,
									BRect frame, const char *text, bool focus = false);
		void					_DrawItem(BView *owner, BRect frame, const char *text,
									bool isSelected = false, bool isEnabled = true,
									bool focus = false);

		void					_UpdateSelection();
		BRect					_FirstCalendarItemFrame() const;
		BRect					_SetNewSelectedDay(const BPoint &where);

								BCalendarView(const BCalendarView &view);
		BCalendarView&			operator=(const BCalendarView &view);

	private:
		struct 					Selection {
									Selection()
										: row(0), column(0) { }

									void SetTo(int32 _row, int32 _column)
									{ row = _row; column = _column; }

									int32 row;
									int32 column;

									Selection& operator=(const Selection &s)
									{ row = s.row; column = s.column; return *this; }

									bool operator==(const Selection &s) const
									{ return row == s.row && column == s.column; }

									bool operator!=(const Selection &s) const
									{ return row != s.row || column != s.column; }
								};
		BRect					_RectOfDay(const Selection &selection) const;

		BMessage				*fSelectionMessage;

		int32					fDay;
		int32					fYear;
		int32					fMonth;

		Selection				fFocusedDay;
		bool					fFocusChanged;
		Selection				fNewFocusedDay;

		Selection				fSelectedDay;
		Selection				fNewSelectedDay;
		bool					fSelectionChanged;

		week_start				fWeekStart;
		bool					fDayNameHeaderVisible;
		bool					fWeekNumberHeaderVisible;

		BString					fDayNames[7];
		BString					fWeekNumbers[6];
		BString					fDayNumbers[6][7];
};


}	// namespace BPrivate


#endif	// _CALENDAR_VIEW_H_
