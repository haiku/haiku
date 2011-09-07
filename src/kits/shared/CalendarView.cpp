/*
 * Copyright 2007-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Julun <host.haiku@gmx.de>
 */


#include "CalendarView.h"

#include <stdlib.h>

#include <LayoutUtils.h>
#include <Window.h>


namespace BPrivate {


static float
FontHeight(const BView* view)
{
	if (!view)
		return 0.0;

	BFont font;
	view->GetFont(&font);
	font_height fheight;
	font.GetHeight(&fheight);
	return ceilf(fheight.ascent + fheight.descent + fheight.leading);
}


// #pragma mark -


BCalendarView::BCalendarView(BRect frame, const char* name, uint32 resizeMask,
	uint32 flags)
	:
	BView(frame, name, resizeMask, flags),
	BInvoker(),
	fSelectionMessage(NULL),
	fDate(),
	fFocusChanged(false),
	fSelectionChanged(false),
	fStartOfWeek((int32)B_WEEKDAY_MONDAY),
	fDayNameHeaderVisible(true),
	fWeekNumberHeaderVisible(true)
{
	_InitObject();
}


BCalendarView::BCalendarView(const char* name, uint32 flags)
	:
	BView(name, flags),
	BInvoker(),
	fSelectionMessage(NULL),
	fDate(),
	fFocusChanged(false),
	fSelectionChanged(false),
	fStartOfWeek((int32)B_WEEKDAY_MONDAY),
	fDayNameHeaderVisible(true),
	fWeekNumberHeaderVisible(true)
{
	_InitObject();
}


BCalendarView::~BCalendarView()
{
	SetSelectionMessage(NULL);
}


BCalendarView::BCalendarView(BMessage* archive)
	:
	BView(archive),
	BInvoker(),
	fSelectionMessage(NULL),
	fDate(archive),
	fFocusChanged(false),
	fSelectionChanged(false),
	fStartOfWeek((int32)B_WEEKDAY_MONDAY),
	fDayNameHeaderVisible(true),
	fWeekNumberHeaderVisible(true)
{
	if (archive->HasMessage("_invokeMsg")) {
		BMessage* invokationMessage = new BMessage;
		archive->FindMessage("_invokeMsg", invokationMessage);
		SetInvocationMessage(invokationMessage);
	}

	if (archive->HasMessage("_selectMsg")) {
		BMessage* selectionMessage = new BMessage;
		archive->FindMessage("selectMsg", selectionMessage);
		SetSelectionMessage(selectionMessage);
	}

	if (archive->FindInt32("_weekStart", &fStartOfWeek) != B_OK)
		fStartOfWeek = (int32)B_WEEKDAY_MONDAY;

	if (archive->FindBool("_dayHeader", &fDayNameHeaderVisible) != B_OK)
		fDayNameHeaderVisible = true;

	if (archive->FindBool("_weekHeader", &fWeekNumberHeaderVisible) != B_OK)
		fWeekNumberHeaderVisible = true;

	_SetupDayNames();
	_SetupDayNumbers();
	_SetupWeekNumbers();
}


BArchivable*
BCalendarView::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "BCalendarView"))
		return new BCalendarView(archive);

	return NULL;
}


status_t
BCalendarView::Archive(BMessage* archive, bool deep) const
{
	status_t status = BView::Archive(archive, deep);

	if (status == B_OK && InvocationMessage())
		status = archive->AddMessage("_invokeMsg", InvocationMessage());

	if (status == B_OK && SelectionMessage())
		status = archive->AddMessage("_selectMsg", SelectionMessage());

	if (status == B_OK)
		status = fDate.Archive(archive);

	if (status == B_OK)
		status = archive->AddInt32("_weekStart", fStartOfWeek);

	if (status == B_OK)
		status = archive->AddBool("_dayHeader", fDayNameHeaderVisible);

	if (status == B_OK)
		status = archive->AddBool("_weekHeader", fWeekNumberHeaderVisible);

	return status;
}


void
BCalendarView::AttachedToWindow()
{
	BView::AttachedToWindow();

	if (!Messenger().IsValid())
		SetTarget(Window(), NULL);
}


void
BCalendarView::FrameResized(float width, float height)
{
	Invalidate(Bounds());
}


void
BCalendarView::Draw(BRect updateRect)
{
	if (LockLooper()) {
		if (fFocusChanged) {
			_DrawFocusRect();
			UnlockLooper();
			return;
		}

		if (fSelectionChanged) {
			_UpdateSelection();
			UnlockLooper();
			return;
		}

		_DrawDays();
		_DrawDayHeader();
		_DrawWeekHeader();

		rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
		SetHighColor(tint_color(background, B_DARKEN_3_TINT));
		StrokeRect(Bounds());

		UnlockLooper();
	}
}


void
BCalendarView::DrawDay(BView* owner, BRect frame, const char* text,
	bool isSelected, bool isEnabled, bool focus)
{
	_DrawItem(owner, frame, text, isSelected, isEnabled, focus);
}


void
BCalendarView::DrawDayName(BView* owner, BRect frame, const char* text)
{
	// we get the full rect, fake this as the internal function
	// shrinks the frame to work properly when drawing a day item
	_DrawItem(owner, frame.InsetByCopy(-1.0, -1.0), text, true);
}


void
BCalendarView::DrawWeekNumber(BView* owner, BRect frame, const char* text)
{
	// we get the full rect, fake this as the internal function
	// shrinks the frame to work properly when drawing a day item
	_DrawItem(owner, frame.InsetByCopy(-1.0, -1.0), text, true);
}


uint32
BCalendarView::SelectionCommand() const
{
	if (SelectionMessage())
		return SelectionMessage()->what;

	return 0;
}


BMessage*
BCalendarView::SelectionMessage() const
{
	return fSelectionMessage;
}


void
BCalendarView::SetSelectionMessage(BMessage* message)
{
	delete fSelectionMessage;
	fSelectionMessage = message;
}


uint32
BCalendarView::InvocationCommand() const
{
	return BInvoker::Command();
}


BMessage*
BCalendarView::InvocationMessage() const
{
	return BInvoker::Message();
}


void
BCalendarView::SetInvocationMessage(BMessage* message)
{
	BInvoker::SetMessage(message);
}


void
BCalendarView::MakeFocus(bool state)
{
	if (IsFocus() == state)
		return;

	BView::MakeFocus(state);

	// TODO: solve this better
	fFocusChanged = true;
	Draw(_RectOfDay(fFocusedDay));
	fFocusChanged = false;
}


status_t
BCalendarView::Invoke(BMessage* message)
{
	bool notify = false;
	uint32 kind = InvokeKind(&notify);

	BMessage clone(kind);
	status_t status = B_BAD_VALUE;

	if (!message && !notify)
		message = Message();

	if (!message) {
		if (!IsWatched())
			return status;
	} else
		clone = *message;

	clone.AddPointer("source", this);
	clone.AddInt64("when", (int64)system_time());
	clone.AddMessenger("be:sender", BMessenger(this));

	int32 year;
	int32 month;
	_GetYearMonthForSelection(fSelectedDay, &year, &month);

	clone.AddInt32("year", fDate.Year());
	clone.AddInt32("month", fDate.Month());
	clone.AddInt32("day", fDate.Day());

	if (message)
		status = BInvoker::Invoke(&clone);

	SendNotices(kind, &clone);

	return status;
}


void
BCalendarView::MouseDown(BPoint where)
{
	if (!IsFocus()) {
		MakeFocus();
		Sync();
		Window()->UpdateIfNeeded();
	}

	BRect frame = Bounds();
	if (fDayNameHeaderVisible)
		frame.top += frame.Height() / 7 - 1.0;

	if (fWeekNumberHeaderVisible)
		frame.left += frame.Width() / 8 - 1.0;

	if (!frame.Contains(where))
		return;

	// try to set to new day
	frame = _SetNewSelectedDay(where);

	// on success
	if (fSelectedDay != fNewSelectedDay) {
		// update focus
		fFocusChanged = true;
		fNewFocusedDay = fNewSelectedDay;
		Draw(_RectOfDay(fFocusedDay));
		fFocusChanged = false;

		// update selection
		fSelectionChanged = true;
		Draw(frame);
		Draw(_RectOfDay(fSelectedDay));
		fSelectionChanged = false;

		// notify that selection changed
		InvokeNotify(SelectionMessage(), B_CONTROL_MODIFIED);
	}

	int32 clicks;
	// on double click invoke
	BMessage* message = Looper()->CurrentMessage();
	if (message->FindInt32("clicks", &clicks) == B_OK && clicks > 1)
		Invoke();
}


void
BCalendarView::KeyDown(const char* bytes, int32 numBytes)
{
	const int32 kRows = 6;
	const int32 kColumns = 7;

	int32 row = fFocusedDay.row;
	int32 column = fFocusedDay.column;

	switch (bytes[0]) {
		case B_LEFT_ARROW:
			column -= 1;
			if (column < 0) {
				column = kColumns - 1;
				row -= 1;
				if (row >= 0)
					fFocusChanged = true;
			} else
				fFocusChanged = true;
			break;

		case B_RIGHT_ARROW:
			column += 1;
			if (column == kColumns) {
				column = 0;
				row += 1;
				if (row < kRows)
					fFocusChanged = true;
			} else
				fFocusChanged = true;
			break;

		case B_UP_ARROW:
			row -= 1;
			if (row >= 0)
				fFocusChanged = true;
			break;

		case B_DOWN_ARROW:
			row += 1;
			if (row < kRows)
				fFocusChanged = true;
			break;

		case B_PAGE_UP:
		{
			BDate date(fDate);
			date.AddMonths(-1);
			SetDate(date);

			Invoke();
			break;
		}

		case B_PAGE_DOWN:
		{
			BDate date(fDate);
			date.AddMonths(1);
			SetDate(date);

			Invoke();
			break;
		}

		case B_RETURN:
		case B_SPACE:
		{
			fSelectionChanged = true;
			BPoint pt = _RectOfDay(fFocusedDay).LeftTop();
			Draw(_SetNewSelectedDay(pt + BPoint(4.0, 4.0)));
			Draw(_RectOfDay(fSelectedDay));
			fSelectionChanged = false;

			Invoke();
			break;
		}

		default:
			BView::KeyDown(bytes, numBytes);
			break;
	}

	if (fFocusChanged) {
		fNewFocusedDay.SetTo(row, column);
		Draw(_RectOfDay(fFocusedDay));
		Draw(_RectOfDay(fNewFocusedDay));
		fFocusChanged = false;
	}
}


void
BCalendarView::ResizeToPreferred()
{
	float width;
	float height;

	GetPreferredSize(&width, &height);
	BView::ResizeTo(width, height);
}


void
BCalendarView::GetPreferredSize(float* width, float* height)
{
	_GetPreferredSize(width, height);
}


BSize
BCalendarView::MaxSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(),
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
}


BSize
BCalendarView::MinSize()
{
	float width, height;
	_GetPreferredSize(&width, &height);
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), BSize(width, height));
}


BSize
BCalendarView::PreferredSize()
{
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), MinSize());
}


int32
BCalendarView::Day() const
{
	return fDate.Day();
}


int32
BCalendarView::Year() const
{
	int32 year;
	_GetYearMonthForSelection(fSelectedDay, &year, NULL);

	return year;
}


int32
BCalendarView::Month() const
{
	int32 month;
	_GetYearMonthForSelection(fSelectedDay, NULL, &month);

	return month;
}


BDate
BCalendarView::Date() const
{
	int32 year;
	int32 month;
	_GetYearMonthForSelection(fSelectedDay, &year, &month);
	return BDate(year, month, fDate.Day());
}


bool
BCalendarView::SetDate(const BDate& date)
{
	if (!date.IsValid())
		return false;

	if (fDate == date)
		return true;

	if (fDate.Year() == date.Year() && fDate.Month() == date.Month()) {
		fDate = date;

		_SetToDay();
		// update focus
		fFocusChanged = true;
		Draw(_RectOfDay(fFocusedDay));
		fFocusChanged = false;

		// update selection
		fSelectionChanged = true;
		Draw(_RectOfDay(fSelectedDay));
		Draw(_RectOfDay(fNewSelectedDay));
		fSelectionChanged = false;
	} else {
		fDate = date;

		_SetupDayNumbers();
		_SetupWeekNumbers();

		BRect frame = Bounds();
		if (fDayNameHeaderVisible)
			frame.top += frame.Height() / 7 - 1.0;

		if (fWeekNumberHeaderVisible)
			frame.left += frame.Width() / 8 - 1.0;

		Draw(frame.InsetBySelf(4.0, 4.0));
	}

	return true;
}


bool
BCalendarView::SetDate(int32 year, int32 month, int32 day)
{
	return SetDate(BDate(year, month, day));
}


BWeekday
BCalendarView::StartOfWeek() const
{
	return BWeekday(fStartOfWeek);
}


void
BCalendarView::SetStartOfWeek(BWeekday startOfWeek)
{
	if (fStartOfWeek == (int32)startOfWeek)
		return;

	fStartOfWeek = (int32)startOfWeek;

	_SetupDayNames();
	_SetupDayNumbers();
	_SetupWeekNumbers();

	Invalidate(Bounds().InsetBySelf(1.0, 1.0));
}


bool
BCalendarView::IsDayNameHeaderVisible() const
{
	return fDayNameHeaderVisible;
}


void
BCalendarView::SetDayNameHeaderVisible(bool visible)
{
	if (fDayNameHeaderVisible == visible)
		return;

	fDayNameHeaderVisible = visible;
	Invalidate(Bounds().InsetBySelf(1.0, 1.0));
}


bool
BCalendarView::IsWeekNumberHeaderVisible() const
{
	return fWeekNumberHeaderVisible;
}


void
BCalendarView::SetWeekNumberHeaderVisible(bool visible)
{
	if (fWeekNumberHeaderVisible == visible)
		return;

	fWeekNumberHeaderVisible = visible;
	Invalidate(Bounds().InsetBySelf(1.0, 1.0));
}


void
BCalendarView::_InitObject()
{
	fDate = BDate::CurrentDate(B_LOCAL_TIME);

	BLocale::Default()->GetStartOfWeek((BWeekday*)&fStartOfWeek);

	_SetupDayNames();
	_SetupDayNumbers();
	_SetupWeekNumbers();
}


void
BCalendarView::_SetToDay()
{
	BDate date(fDate.Year(), fDate.Month(), 1);
	if (!date.IsValid())
		return;

	const int32 firstDayOffset = (7 + date.DayOfWeek() - fStartOfWeek) % 7;

	int32 day = 1 - firstDayOffset;
	for (int32 row = 0; row < 6; ++row) {
		for (int32 column = 0; column < 7; ++column) {
			if (day == fDate.Day()) {
				fNewFocusedDay.SetTo(row, column);
				fNewSelectedDay.SetTo(row, column);
				return;
			}
			day++;
		}
	}

	fNewFocusedDay.SetTo(0, 0);
	fNewSelectedDay.SetTo(0, 0);
}


void
BCalendarView::_GetYearMonthForSelection(const Selection& selection,
	int32* year, int32* month) const
{
	BDate startOfMonth(fDate.Year(), fDate.Month(), 1);
	const int32 firstDayOffset
		= (7 + startOfMonth.DayOfWeek() - fStartOfWeek) % 7;
	const int32 daysInMonth = startOfMonth.DaysInMonth();

	BDate date(fDate);
	const int32 dayOffset = selection.row * 7 + selection.column;
	if (dayOffset < firstDayOffset)
		date.AddMonths(-1);
	else if (dayOffset >= firstDayOffset + daysInMonth)
		date.AddMonths(1);
	if (year != NULL)
		*year = date.Year();
	if (month != NULL)
		*month = date.Month();
}


void
BCalendarView::_GetPreferredSize(float* _width, float* _height)
{
	BFont font;
	GetFont(&font);
	font_height fontHeight;
	font.GetHeight(&fontHeight);

	const float height = FontHeight(this) + 4.0;

	int32 rows = 7;
	if (!fDayNameHeaderVisible)
		rows = 6;

	// height = font height * rows + 8 px border
	*_height = height * rows + 8.0;

	float width = 0.0;
	for (int32 column = 0; column < 7; ++column) {
		float tmp = StringWidth(fDayNames[column].String()) + 2.0;
		width = tmp > width ? tmp : width;
	}

	int32 columns = 8;
	if (!fWeekNumberHeaderVisible)
		columns = 7;

	// width = max width day name * 8 column + 8 px border
	*_width = width * columns + 8.0;
}


void
BCalendarView::_SetupDayNames()
{
	for (int32 i = 0; i <= 6; ++i)
		fDayNames[i] = fDate.ShortDayName(1 + (fStartOfWeek - 1 + i) % 7);
}


void
BCalendarView::_SetupDayNumbers()
{
	BDate startOfMonth(fDate.Year(), fDate.Month(), 1);
	if (!startOfMonth.IsValid())
		return;

	fFocusedDay.SetTo(0, 0);
	fSelectedDay.SetTo(0, 0);
	fNewFocusedDay.SetTo(0, 0);

	const int32 daysInMonth = startOfMonth.DaysInMonth();
	const int32 firstDayOffset
		= (7 + startOfMonth.DayOfWeek() - fStartOfWeek) % 7;

	// calc the last day one month before
	BDate lastDayInMonthBefore(startOfMonth);
	lastDayInMonthBefore.AddDays(-1);
	const int32 lastDayBefore = lastDayInMonthBefore.DaysInMonth();

	int32 counter = 0;
	int32 firstDayAfter = 1;
	for (int32 row = 0; row < 6; ++row) {
		for (int32 column = 0; column < 7; ++column) {
			int32 day = 1 + counter - firstDayOffset;
			if (counter < firstDayOffset)
				day += lastDayBefore;
			else if (counter >= firstDayOffset + daysInMonth)
				day = firstDayAfter++;
			else if (day == fDate.Day()) {
				fFocusedDay.SetTo(row, column);
				fSelectedDay.SetTo(row, column);
				fNewFocusedDay.SetTo(row, column);
			}
			counter++;
			fDayNumbers[row][column].Truncate(0);
			fDayNumbers[row][column] << day;
		}
	}
}


void
BCalendarView::_SetupWeekNumbers()
{
	BDate date(fDate.Year(), fDate.Month(), 1);
	if (!date.IsValid())
		return;

	for (int32 row = 0; row < 6; ++row) {
		fWeekNumbers[row].SetTo("");
		fWeekNumbers[row] << date.WeekNumber();
		date.AddDays(7);
	}
}


void
BCalendarView::_DrawDay(int32 currRow, int32 currColumn, int32 row,
	int32 column, int32 counter, BRect frame, const char* text, bool focus)
{
	BDate startOfMonth(fDate.Year(), fDate.Month(), 1);
	const int32 firstDayOffset
		= (7 + startOfMonth.DayOfWeek() - fStartOfWeek) % 7;
	const int32 daysMonth = startOfMonth.DaysInMonth();

	bool enabled = true;
	bool selected = false;
	// check for the current date
	if (currRow == row  && currColumn == column) {
		selected = true;	// draw current date selected
		if (counter <= firstDayOffset || counter > firstDayOffset + daysMonth) {
			enabled = false;	// days of month before or after
			selected = false;	// not selected but able to get focus
		}
	} else {
		if (counter <= firstDayOffset || counter > firstDayOffset + daysMonth)
			enabled = false;	// days of month before or after
	}

	DrawDay(this, frame, text, selected, enabled, focus);
}


void
BCalendarView::_DrawDays()
{
	BRect frame = _FirstCalendarItemFrame();

	const int32 currRow = fSelectedDay.row;
	const int32 currColumn = fSelectedDay.column;

	const bool isFocus = IsFocus();
	const int32 focusRow = fFocusedDay.row;
	const int32 focusColumn = fFocusedDay.column;

	int32 counter = 0;
	for (int32 row = 0; row < 6; ++row) {
		BRect tmp = frame;
		for (int32 column = 0; column < 7; ++column) {
			counter++;
			const char* day = fDayNumbers[row][column].String();
			bool focus = isFocus && focusRow == row && focusColumn == column;
			_DrawDay(currRow, currColumn, row, column, counter, tmp, day,
				focus);

			tmp.OffsetBy(tmp.Width(), 0.0);
		}
		frame.OffsetBy(0.0, frame.Height());
	}
}


void
BCalendarView::_DrawFocusRect()
{
	BRect frame = _FirstCalendarItemFrame();

	const int32 currRow = fSelectedDay.row;
	const int32 currColumn = fSelectedDay.column;

	const int32 focusRow = fFocusedDay.row;
	const int32 focusColumn = fFocusedDay.column;

	int32 counter = 0;
	for (int32 row = 0; row < 6; ++row) {
		BRect tmp = frame;
		for (int32 column = 0; column < 7; ++column) {
			counter++;
			if (fNewFocusedDay.row == row && fNewFocusedDay.column == column) {
				fFocusedDay.SetTo(row, column);

				bool focus = IsFocus() && true;
				const char* day = fDayNumbers[row][column].String();
				_DrawDay(currRow, currColumn, row, column, counter, tmp, day,
					focus);
			} else if (focusRow == row && focusColumn == column) {
				const char* day = fDayNumbers[row][column].String();
				_DrawDay(currRow, currColumn, row, column, counter, tmp, day,
					false);
			}
			tmp.OffsetBy(tmp.Width(), 0.0);
		}
		frame.OffsetBy(0.0, frame.Height());
	}
}


void
BCalendarView::_DrawDayHeader()
{
	if (!fDayNameHeaderVisible)
		return;

	int32 offset = 1;
	int32 columns = 8;
	if (!fWeekNumberHeaderVisible) {
		offset = 0;
		columns = 7;
	}

	BRect frame = Bounds();
	frame.right = frame.Width() / columns - 1.0;
	frame.bottom = frame.Height() / 7.0 - 2.0;
	frame.OffsetBy(4.0, 4.0);

	for (int32 i = 0; i < columns; ++i) {
		if (i == 0 && fWeekNumberHeaderVisible) {
			DrawDayName(this, frame, "");
			frame.OffsetBy(frame.Width(), 0.0);
			continue;
		}
		DrawDayName(this, frame, fDayNames[i - offset].String());
		frame.OffsetBy(frame.Width(), 0.0);
	}
}


void
BCalendarView::_DrawWeekHeader()
{
	if (!fWeekNumberHeaderVisible)
		return;

	int32 rows = 7;
	if (!fDayNameHeaderVisible)
		rows = 6;

	BRect frame = Bounds();
	frame.right = frame.Width() / 8.0 - 2.0;
	frame.bottom = frame.Height() / rows - 1.0;

	float offsetY = 4.0;
	if (fDayNameHeaderVisible)
		offsetY += frame.Height();

	frame.OffsetBy(4.0, offsetY);

	for (int32 row = 0; row < 6; ++row) {
		DrawWeekNumber(this, frame, fWeekNumbers[row].String());
		frame.OffsetBy(0.0, frame.Height());
	}
}


void
BCalendarView::_DrawItem(BView* owner, BRect frame, const char* text,
	bool isSelected, bool isEnabled, bool focus)
{
	rgb_color lColor = LowColor();
	rgb_color highColor = HighColor();

	rgb_color lowColor = { 255, 255, 255, 255 };

	if (isSelected) {
		SetHighColor(tint_color(lowColor, B_DARKEN_2_TINT));
		SetLowColor(HighColor());
	} else
		SetHighColor(lowColor);

	FillRect(frame.InsetByCopy(1.0, 1.0));

	if (focus) {
		SetHighColor(keyboard_navigation_color());
		StrokeRect(frame.InsetByCopy(1.0, 1.0));
	}

	rgb_color black = { 0, 0, 0, 255 };
	SetHighColor(black);
	if (!isEnabled)
		SetHighColor(tint_color(black, B_LIGHTEN_2_TINT));

	float offsetH = frame.Width() / 2.0;
	float offsetV = frame.Height() / 2.0 + FontHeight(owner) / 2.0 - 2.0;

	DrawString(text, BPoint(frame.right - offsetH - StringWidth(text) / 2.0,
			frame.top + offsetV));

	SetLowColor(lColor);
	SetHighColor(highColor);
}


void
BCalendarView::_UpdateSelection()
{
	BRect frame = _FirstCalendarItemFrame();

	const int32 currRow = fSelectedDay.row;
	const int32 currColumn = fSelectedDay.column;

	const int32 focusRow = fFocusedDay.row;
	const int32 focusColumn = fFocusedDay.column;

	int32 counter = 0;
	for (int32 row = 0; row < 6; ++row) {
		BRect tmp = frame;
		for (int32 column = 0; column < 7; ++column) {
			counter++;
			if (fNewSelectedDay.row == row
				&& fNewSelectedDay.column == column) {
				fSelectedDay.SetTo(row, column);

				const char* day = fDayNumbers[row][column].String();
				bool focus = IsFocus() && focusRow == row
					&& focusColumn == column;
				_DrawDay(row, column, row, column, counter, tmp, day, focus);
			} else if (currRow == row && currColumn == column) {
				const char* day = fDayNumbers[row][column].String();
				bool focus = IsFocus() && focusRow == row
					&& focusColumn == column;
				_DrawDay(currRow, currColumn, -1, -1, counter, tmp, day, focus);
			}
			tmp.OffsetBy(tmp.Width(), 0.0);
		}
		frame.OffsetBy(0.0, frame.Height());
	}
}


BRect
BCalendarView::_FirstCalendarItemFrame() const
{
	int32 rows = 7;
	int32 columns = 8;

	if (!fDayNameHeaderVisible)
		rows = 6;

	if (!fWeekNumberHeaderVisible)
		columns = 7;

	BRect frame = Bounds();
	frame.right = frame.Width() / columns - 1.0;
	frame.bottom = frame.Height() / rows - 1.0;

	float offsetY = 4.0;
	if (fDayNameHeaderVisible)
		offsetY += frame.Height();

	float offsetX = 4.0;
	if (fWeekNumberHeaderVisible)
		offsetX += frame.Width();

	return frame.OffsetBySelf(offsetX, offsetY);
}


BRect
BCalendarView::_SetNewSelectedDay(const BPoint& where)
{
	BRect frame = _FirstCalendarItemFrame();

	int32 counter = 0;
	for (int32 row = 0; row < 6; ++row) {
		BRect tmp = frame;
		for (int32 column = 0; column < 7; ++column) {
			counter++;
			if (tmp.Contains(where)) {
				fNewSelectedDay.SetTo(row, column);
				int32 year;
				int32 month;
				_GetYearMonthForSelection(fNewSelectedDay, &year, &month);
				if (month == fDate.Month()) {
					// only change date if a day in the current month has been
					// selected
					int32 day = atoi(fDayNumbers[row][column].String());
					fDate.SetDate(year, month, day);
				}
				return tmp;
			}
			tmp.OffsetBy(tmp.Width(), 0.0);
		}
		frame.OffsetBy(0.0, frame.Height());
	}

	return frame;
}


BRect
BCalendarView::_RectOfDay(const Selection& selection) const
{
	BRect frame = _FirstCalendarItemFrame();

	int32 counter = 0;
	for (int32 row = 0; row < 6; ++row) {
		BRect tmp = frame;
		for (int32 column = 0; column < 7; ++column) {
			counter++;
			if (selection.row == row && selection.column == column)
				return tmp;
			tmp.OffsetBy(tmp.Width(), 0.0);
		}
		frame.OffsetBy(0.0, frame.Height());
	}

	return frame;
}


}	// namespace BPrivate
