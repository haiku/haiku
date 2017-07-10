/*
 * Copyright 2007-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CALENDAR_VIEW_H_
#define _CALENDAR_VIEW_H_


#include "DateTime.h"


#include <DateFormat.h>
#include <Invoker.h>
#include <List.h>
#include <Locale.h>
#include <String.h>
#include <View.h>


class BMessage;


namespace BPrivate {


class BCalendarView : public BView, public BInvoker {
public:
								BCalendarView(BRect frame, const char* name,
									uint32 resizeMask = B_FOLLOW_LEFT_TOP,
									uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS
										| B_NAVIGABLE | B_PULSE_NEEDED);

								BCalendarView(const char* name,
									uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS
										| B_NAVIGABLE | B_PULSE_NEEDED);

	virtual						~BCalendarView();

								BCalendarView(BMessage* archive);
	static 	BArchivable*		Instantiate(BMessage* archive);
	virtual status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual void				AttachedToWindow();

	virtual void				FrameResized(float width, float height);

	virtual void				Draw(BRect updateRect);

	virtual void				DrawDay(BView* owner, BRect frame,
									const char* text, bool isSelected = false,
									bool isEnabled = true, bool focus = false,
									bool highlight = false);
	virtual void				DrawDayName(BView* owner, BRect frame,
									const char* text);
	virtual void				DrawWeekNumber(BView* owner, BRect frame,
									const char* text);

			uint32				SelectionCommand() const;
			BMessage*			SelectionMessage() const;
	virtual void				SetSelectionMessage(BMessage* message);

			uint32				InvocationCommand() const;
			BMessage*			InvocationMessage() const;
	virtual void				SetInvocationMessage(BMessage* message);

	virtual void				MakeFocus(bool state = true);
	virtual status_t			Invoke(BMessage* message = NULL);

	virtual void				MouseDown(BPoint where);

	virtual void				KeyDown(const char* bytes, int32 numBytes);

	virtual void				Pulse();

	virtual void				ResizeToPreferred();
	virtual void				GetPreferredSize(float* width, float* height);

	virtual	BSize				MaxSize();
	virtual	BSize				MinSize();
	virtual	BSize				PreferredSize();

			int32				Day() const;
			int32				Month() const;
			int32				Year() const;

			bool				SetDay(int32 day);
			bool				SetMonth(int32 month);
			bool				SetYear(int32 year);

			BDate				Date() const;
			bool				SetDate(const BDate& date);
			bool				SetDate(int32 year, int32 month, int32 day);

			BWeekday			StartOfWeek() const;
			void				SetStartOfWeek(BWeekday startOfWeek);

			bool				IsDayNameHeaderVisible() const;
			void				SetDayNameHeaderVisible(bool visible);

			bool				IsWeekNumberHeaderVisible() const;
			void				SetWeekNumberHeaderVisible(bool visible);

private:
			struct 				Selection {
									Selection()
										: row(0), column(0)
									{
									}

									void
									SetTo(int32 _row, int32 _column)
									{
										row = _row;
										column = _column;
									}

									int32 row;
									int32 column;

									Selection& operator=(const Selection& s)
									{
										row = s.row;
										column = s.column;
										return *this;
									}

									bool operator==(const Selection& s) const
									{
										return row == s.row
											&& column == s.column;
									}

									bool operator!=(const Selection& s) const
									{
										return row != s.row
											|| column != s.column;
									}
								};

			void				_InitObject();

			void				_SetToDay();
			void				_SetToCurrentDay();
			void				_GetYearMonthForSelection(
									const Selection& selection, int32* year,
									int32* month) const;
			void				_GetPreferredSize(float* width, float* height);

			void				_SetupDayNames();
			void				_SetupDayNumbers();
			void				_SetupWeekNumbers();

			void				_DrawDays();
			void				_DrawFocusRect();
			void				_DrawDayHeader();
			void				_DrawWeekHeader();
			void				_DrawDay(int32 curRow, int32 curColumn,
									int32 row, int32 column, int32 counter,
									BRect frame, const char* text,
									bool focus = false, bool highlight = false);
			void				_DrawItem(BView* owner, BRect frame,
									const char* text, bool isSelected = false,
									bool isEnabled = true, bool focus = false,
									bool highlight = false);

			void				_UpdateSelection();
			void				_UpdateCurrentDay();
			void				_UpdateCurrentDate();

			BRect				_FirstCalendarItemFrame() const;
			BRect				_SetNewSelectedDay(const BPoint& where);

			BRect				_RectOfDay(const Selection& selection) const;

private:
			BMessage*			fSelectionMessage;

			BDate				fDate;
			BDate				fCurrentDate;

			Selection			fFocusedDay;
			Selection			fNewFocusedDay;
			bool				fFocusChanged;

			Selection			fSelectedDay;
			Selection			fNewSelectedDay;
			bool				fSelectionChanged;

			Selection			fCurrentDay;
			Selection			fNewCurrentDay;
			bool				fCurrentDayChanged;

			int32				fStartOfWeek;
			bool				fDayNameHeaderVisible;
			bool				fWeekNumberHeaderVisible;

			BString				fDayNames[7];
			BString				fWeekNumbers[6];
			BString				fDayNumbers[6][7];

			// hide copy constructor & assignment
								BCalendarView(const BCalendarView& view);
			BCalendarView&		operator=(const BCalendarView& view);
};


}	// namespace BPrivate


#endif	// _CALENDAR_VIEW_H_
