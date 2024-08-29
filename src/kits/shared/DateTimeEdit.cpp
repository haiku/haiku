/*
 * Copyright 2004-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		McCall <mccall@@digitalparadise.co.uk>
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 *		Clemens <mail@Clemens-Zeidler.de>
 *		Adrien Destugues <pulkomandy@pulkomandy.cx>
 *		Hamish Morrison <hamish@lavabit.com>
 */


#include "DateTimeEdit.h"

#include <stdlib.h>

#include <ControlLook.h>
#include <DateFormat.h>
#include <LayoutUtils.h>
#include <List.h>
#include <Locale.h>
#include <String.h>
#include <Window.h>


namespace BPrivate {


const uint32 kArrowAreaWidth = 16;


TimeEdit::TimeEdit(const char* name, uint32 sections, BMessage* message)
	:
	SectionEdit(name, sections, message),
	fLastKeyDownTime(0),
	fFields(NULL),
	fFieldCount(0),
	fFieldPositions(NULL),
	fFieldPosCount(0)
{
	InitView();
}


TimeEdit::~TimeEdit()
{
	free(fFieldPositions);
	free(fFields);
}


void
TimeEdit::KeyDown(const char* bytes, int32 numBytes)
{
	if (IsEnabled() == false)
		return;
	SectionEdit::KeyDown(bytes, numBytes);

	// only accept valid input
	int32 number = atoi(bytes);
	if (number < 0 || bytes[0] < '0')
		return;

	int32 section = FocusIndex();
	if (section < 0 || section > 2)
		return;

	bigtime_t currentTime = system_time();
	if (currentTime - fLastKeyDownTime < 1000000) {
		int32 doubleDigit = number + fLastKeyDownInt * 10;
		if (_IsValidDoubleDigit(doubleDigit))
			number = doubleDigit;
		fLastKeyDownTime = 0;
	} else {
		fLastKeyDownTime = currentTime;
		fLastKeyDownInt = number;
	}

	// update display value
	fHoldValue = number;
	_CheckRange();
	_UpdateFields();

	// send message to change time
	Invoke();
}


void
TimeEdit::InitView()
{
	// make sure we call the base class method, as it
	// will create the arrow bitmaps and the section list
	fTime = BDateTime::CurrentDateTime(B_LOCAL_TIME);
	_UpdateFields();
}


void
TimeEdit::DrawSection(uint32 index, BRect bounds, bool hasFocus)
{
	if (fFieldPositions == NULL || index * 2 + 1 >= (uint32)fFieldPosCount)
		return;

	if (hasFocus)
		SetLowColor(mix_color(ui_color(B_CONTROL_HIGHLIGHT_COLOR),
			ui_color(B_DOCUMENT_BACKGROUND_COLOR), 192));
	else
		SetLowUIColor(B_DOCUMENT_BACKGROUND_COLOR);

	BString field;
	fText.CopyCharsInto(field, fFieldPositions[index * 2],
		fFieldPositions[index * 2 + 1] - fFieldPositions[index * 2]);

	BPoint point(bounds.LeftBottom());
	point.y -= bounds.Height() / 2.0 - 6.0;
	point.x += (bounds.Width() - StringWidth(field)) / 2;
	FillRect(bounds, B_SOLID_LOW);
	DrawString(field, point);
}


void
TimeEdit::DrawSeparator(uint32 index, BRect bounds)
{
	if (fFieldPositions == NULL || index * 2 + 2 >= (uint32)fFieldPosCount)
		return;

	BString field;
	fText.CopyCharsInto(field, fFieldPositions[index * 2 + 1],
		fFieldPositions[index * 2 + 2] - fFieldPositions[index * 2 + 1]);

	BPoint point(bounds.LeftBottom());
	point.y -= bounds.Height() / 2.0 - 6.0;
	point.x += (bounds.Width() - StringWidth(field)) / 2;
	DrawString(field, point);
}


float
TimeEdit::SeparatorWidth()
{
	return 10.0f;
}


float
TimeEdit::MinSectionWidth()
{
	return be_plain_font->StringWidth("00");
}


void
TimeEdit::SectionFocus(uint32 index)
{
	fLastKeyDownTime = 0;
	fFocus = index;
	fHoldValue = _SectionValue(index);
	Draw(Bounds());
}


void
TimeEdit::SetTime(int32 hour, int32 minute, int32 second)
{
	// make sure to update date upon overflow
	if (hour == 0 && minute == 0 && second == 0)
		fTime = BDateTime::CurrentDateTime(B_LOCAL_TIME);

	fTime.SetTime(BTime(hour, minute, second));

	if (LockLooper()) {
		_UpdateFields();
		UnlockLooper();
	}

	Invalidate(Bounds());
}


BTime
TimeEdit::GetTime()
{
	return fTime.Time();
}


void
TimeEdit::DoUpPress()
{
	if (fFocus == -1)
		SectionFocus(0);

	// update displayed value
	fHoldValue += 1;

	_CheckRange();
	_UpdateFields();

	// send message to change time
	Invoke();
}


void
TimeEdit::DoDownPress()
{
	if (fFocus == -1)
		SectionFocus(0);

	// update display value
	fHoldValue -= 1;

	_CheckRange();
	_UpdateFields();

	Invoke();
}


void
TimeEdit::PopulateMessage(BMessage* message)
{
	if (fFocus < 0 || fFocus >= fFieldCount)
		return;

	message->AddBool("time", true);
	message->AddInt32("hour", fTime.Time().Hour());
	message->AddInt32("minute", fTime.Time().Minute());
	message->AddInt32("second", fTime.Time().Second());
}


void
TimeEdit::_UpdateFields()
{
	time_t time = fTime.Time_t();

	if (fFieldPositions != NULL) {
		free(fFieldPositions);
		fFieldPositions = NULL;
	}
	fTimeFormat.Format(fText, fFieldPositions, fFieldPosCount, time,
		B_MEDIUM_TIME_FORMAT);

	if (fFields != NULL) {
		free(fFields);
		fFields = NULL;
	}
	fTimeFormat.GetTimeFields(fFields, fFieldCount, B_MEDIUM_TIME_FORMAT);
}


void
TimeEdit::_CheckRange()
{
	if (fFocus < 0 || fFocus >= fFieldCount)
		return;

	int32 value = fHoldValue;
	switch (fFields[fFocus]) {
		case B_DATE_ELEMENT_HOUR:
			if (value > 23)
				value = 0;
			else if (value < 0)
				value = 23;

			fTime.SetTime(BTime(value, fTime.Time().Minute(),
				fTime.Time().Second()));
			break;

		case B_DATE_ELEMENT_MINUTE:
			if (value> 59)
				value = 0;
			else if (value < 0)
				value = 59;

			fTime.SetTime(BTime(fTime.Time().Hour(), value,
				fTime.Time().Second()));
			break;

		case B_DATE_ELEMENT_SECOND:
			if (value > 59)
				value = 0;
			else if (value < 0)
				value = 59;

			fTime.SetTime(BTime(fTime.Time().Hour(), fTime.Time().Minute(),
				value));
			break;

		case B_DATE_ELEMENT_AM_PM:
			value = fTime.Time().Hour();
			if (value < 13)
				value += 12;
			else
				value -= 12;
			if (value == 24)
				value = 0;

			// modify hour value to reflect change in am/ pm
			fTime.SetTime(BTime(value, fTime.Time().Minute(),
				fTime.Time().Second()));
			break;

		default:
			return;
	}


	fHoldValue = value;
	Invalidate(Bounds());
}


bool
TimeEdit::_IsValidDoubleDigit(int32 value)
{
	if (fFocus < 0 || fFocus >= fFieldCount)
		return false;

	bool isInRange = false;
	switch (fFields[fFocus]) {
		case B_DATE_ELEMENT_HOUR:
			if (value <= 23)
				isInRange = true;
			break;

		case B_DATE_ELEMENT_MINUTE:
			if (value <= 59)
				isInRange = true;
			break;

		case B_DATE_ELEMENT_SECOND:
			if (value <= 59)
				isInRange = true;
			break;

		default:
			break;
	}

	return isInRange;
}


int32
TimeEdit::_SectionValue(int32 index) const
{
	if (index < 0 || index >= fFieldCount)
		return 0;

	int32 value;
	switch (fFields[index]) {
		case B_DATE_ELEMENT_HOUR:
			value = fTime.Time().Hour();
			break;

		case B_DATE_ELEMENT_MINUTE:
			value = fTime.Time().Minute();
			break;

		case B_DATE_ELEMENT_SECOND:
			value = fTime.Time().Second();
			break;

		default:
			value = 0;
			break;
	}

	return value;
}


float
TimeEdit::PreferredHeight()
{
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	return ceilf((fontHeight.ascent + fontHeight.descent) * 1.4);
}


// #pragma mark -


DateEdit::DateEdit(const char* name, uint32 sections, BMessage* message)
	:
	SectionEdit(name, sections, message),
	fFields(NULL),
	fFieldCount(0),
	fFieldPositions(NULL),
	fFieldPosCount(0)
{
	InitView();
}


DateEdit::~DateEdit()
{
	free(fFieldPositions);
	free(fFields);
}


void
DateEdit::KeyDown(const char* bytes, int32 numBytes)
{
	if (IsEnabled() == false)
		return;
	SectionEdit::KeyDown(bytes, numBytes);

	// only accept valid input
	int32 number = atoi(bytes);
	if (number < 0 || bytes[0] < '0')
		return;

	int32 section = FocusIndex();
	if (section < 0 || section > 2)
		return;

	bigtime_t currentTime = system_time();
	if (currentTime - fLastKeyDownTime < 1000000) {
		int32 doubleDigit = number + fLastKeyDownInt * 10;
		if (_IsValidDoubleDigit(doubleDigit))
			number = doubleDigit;
		fLastKeyDownTime = 0;
	} else {
		fLastKeyDownTime = currentTime;
		fLastKeyDownInt = number;
	}

	// if year add 2000

	if (fFields[section] == B_DATE_ELEMENT_YEAR) {
		int32 oldCentury = int32(fHoldValue / 100) * 100;
		if (number < 10 && oldCentury == 1900)
			number += 70;
		number += oldCentury;
	}
	fHoldValue = number;

	// update display value
	_CheckRange();
	_UpdateFields();

	// send message to change time
	Invoke();
}


void
DateEdit::InitView()
{
	// make sure we call the base class method, as it
	// will create the arrow bitmaps and the section list
	fDate = BDate::CurrentDate(B_LOCAL_TIME);
	_UpdateFields();
}


void
DateEdit::DrawSection(uint32 index, BRect bounds, bool hasFocus)
{
	if (fFieldPositions == NULL || index * 2 + 1 >= (uint32)fFieldPosCount)
		return;

	if (hasFocus)
		SetLowColor(mix_color(ui_color(B_CONTROL_HIGHLIGHT_COLOR),
			ui_color(B_DOCUMENT_BACKGROUND_COLOR), 192));
	else
		SetLowUIColor(B_DOCUMENT_BACKGROUND_COLOR);

	BString field;
	fText.CopyCharsInto(field, fFieldPositions[index * 2],
		fFieldPositions[index * 2 + 1] - fFieldPositions[index * 2]);

	BPoint point(bounds.LeftBottom());
	point.y -= bounds.Height() / 2.0 - 6.0;
	point.x += (bounds.Width() - StringWidth(field)) / 2;
	FillRect(bounds, B_SOLID_LOW);
	DrawString(field, point);
}


void
DateEdit::DrawSeparator(uint32 index, BRect bounds)
{
	if (index >= 2)
		return;

	if (fFieldPositions == NULL || index * 2 + 2 >= (uint32)fFieldPosCount)
		return;

	BString field;
	fText.CopyCharsInto(field, fFieldPositions[index * 2 + 1],
		fFieldPositions[index * 2 + 2] - fFieldPositions[index * 2 + 1]);

	BPoint point(bounds.LeftBottom());
	point.y -= bounds.Height() / 2.0 - 6.0;
	point.x += (bounds.Width() - StringWidth(field)) / 2;
	DrawString(field, point);
}


void
DateEdit::SectionFocus(uint32 index)
{
	fLastKeyDownTime = 0;
	fFocus = index;
	fHoldValue = _SectionValue(index);
	Draw(Bounds());
}


float
DateEdit::MinSectionWidth()
{
	return be_plain_font->StringWidth("00");
}


float
DateEdit::SeparatorWidth()
{
	return 10.0f;
}


void
DateEdit::SetDate(int32 year, int32 month, int32 day)
{
	fDate.SetDate(year, month, day);

	if (LockLooper()) {
		_UpdateFields();
		UnlockLooper();
	}

	Invalidate(Bounds());
}


BDate
DateEdit::GetDate()
{
	return fDate;
}


void
DateEdit::DoUpPress()
{
	if (fFocus == -1)
		SectionFocus(0);

	// update displayed value
	fHoldValue += 1;

	_CheckRange();
	_UpdateFields();

	// send message to change Date
	Invoke();
}


void
DateEdit::DoDownPress()
{
	if (fFocus == -1)
		SectionFocus(0);

	// update display value
	fHoldValue -= 1;

	_CheckRange();
	_UpdateFields();

	// send message to change Date
	Invoke();
}


void
DateEdit::PopulateMessage(BMessage* message)
{
	if (fFocus < 0 || fFocus >= fFieldCount)
		return;

	message->AddBool("time", false);
	message->AddInt32("year", fDate.Year());
	message->AddInt32("month", fDate.Month());
	message->AddInt32("day", fDate.Day());
}


void
DateEdit::_UpdateFields()
{
	time_t time = BDateTime(fDate, BTime()).Time_t();

	if (fFieldPositions != NULL) {
		free(fFieldPositions);
		fFieldPositions = NULL;
	}

	fDateFormat.Format(fText, fFieldPositions, fFieldPosCount, time,
		B_SHORT_DATE_FORMAT);

	if (fFields != NULL) {
		free(fFields);
		fFields = NULL;
	}
	fDateFormat.GetFields(fFields, fFieldCount, B_SHORT_DATE_FORMAT);
}


void
DateEdit::_CheckRange()
{
	if (fFocus < 0 || fFocus >= fFieldCount)
		return;

	int32 value = fHoldValue;
	switch (fFields[fFocus]) {
		case B_DATE_ELEMENT_DAY:
		{
			int32 days = fDate.DaysInMonth();
			if (value > days)
				value = 1;
			else if (value < 1)
				value = days;

			fDate.SetDate(fDate.Year(), fDate.Month(), value);
			break;
		}

		case B_DATE_ELEMENT_MONTH:
		{
			if (value > 12)
				value = 1;
			else if (value < 1)
				value = 12;

			int32 day = fDate.Day();
			fDate.SetDate(fDate.Year(), value, 1);

			// changing between months with differing amounts of days
			while (day > fDate.DaysInMonth())
				day--;
			fDate.SetDate(fDate.Year(), value, day);
			break;
		}

		case B_DATE_ELEMENT_YEAR:
			fDate.SetDate(value, fDate.Month(), fDate.Day());
			break;

		default:
			return;
	}

	fHoldValue = value;
	Invalidate(Bounds());
}


bool
DateEdit::_IsValidDoubleDigit(int32 value)
{
	if (fFocus < 0 || fFocus >= fFieldCount)
		return false;

	bool isInRange = false;
	switch (fFields[fFocus]) {
		case B_DATE_ELEMENT_DAY:
		{
			int32 days = fDate.DaysInMonth();
			if (value >= 1 && value <= days)
				isInRange = true;
			break;
		}

		case B_DATE_ELEMENT_MONTH:
		{
			if (value >= 1 && value <= 12)
				isInRange = true;
			break;
		}

		case B_DATE_ELEMENT_YEAR:
		{
			int32 year = int32(fHoldValue / 100) * 100 + value;
			if (year >= 2000)
				isInRange = true;
			break;
		}

		default:
			break;
	}

	return isInRange;
}


int32
DateEdit::_SectionValue(int32 index) const
{
	if (index < 0 || index >= fFieldCount)
		return 0;

	int32 value = 0;
	switch (fFields[index]) {
		case B_DATE_ELEMENT_YEAR:
			value = fDate.Year();
			break;

		case B_DATE_ELEMENT_MONTH:
			value = fDate.Month();
			break;

		case B_DATE_ELEMENT_DAY:
			value = fDate.Day();
			break;

		default:
			break;
	}

	return value;
}


float
DateEdit::PreferredHeight()
{
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	return ceilf((fontHeight.ascent + fontHeight.descent) * 1.4);
}


// #pragma mark -


SectionEdit::SectionEdit(const char* name, uint32 sections, BMessage* message)
	:
	BControl(name, NULL, message, B_WILL_DRAW | B_NAVIGABLE),
	fFocus(-1),
	fSectionCount(sections),
	fHoldValue(0)
{
}


SectionEdit::~SectionEdit()
{
}


void
SectionEdit::AttachedToWindow()
{
	BControl::AttachedToWindow();

	// Low colors are set in Draw() methods.
	SetHighUIColor(B_DOCUMENT_TEXT_COLOR);
}


void
SectionEdit::Draw(BRect updateRect)
{
	DrawBorder(updateRect);

	for (uint32 idx = 0; idx < fSectionCount; idx++) {
		DrawSection(idx, FrameForSection(idx),
			((uint32)fFocus == idx) && IsFocus());
		if (idx < fSectionCount - 1)
			DrawSeparator(idx, FrameForSeparator(idx));
	}
}


void
SectionEdit::MouseDown(BPoint where)
{
	if (IsEnabled() == false)
		return;

	MakeFocus(true);

	if (fUpRect.Contains(where))
		DoUpPress();
	else if (fDownRect.Contains(where))
		DoDownPress();
	else if (fSectionCount > 0) {
		for (uint32 idx = 0; idx < fSectionCount; idx++) {
			if (FrameForSection(idx).Contains(where)) {
				SectionFocus(idx);
				return;
			}
		}
	}
}


BSize
SectionEdit::MaxSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(),
		BSize(B_SIZE_UNLIMITED, PreferredHeight()));
}


BSize
SectionEdit::MinSize()
{
	BSize minSize;
	minSize.height = PreferredHeight();
	minSize.width = (SeparatorWidth() + MinSectionWidth())
		* fSectionCount;
	return BLayoutUtils::ComposeSize(ExplicitMinSize(),
		minSize);
}


BSize
SectionEdit::PreferredSize()
{
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(),
		MinSize());
}


BRect
SectionEdit::FrameForSection(uint32 index)
{
	BRect area = SectionArea();
	float sepWidth = SeparatorWidth();

	float width = (area.Width() -
		sepWidth * (fSectionCount - 1))
		/ fSectionCount;
	area.left += index * (width + sepWidth);
	area.right = area.left + width;

	return area;
}


BRect
SectionEdit::FrameForSeparator(uint32 index)
{
	BRect area = SectionArea();
	float sepWidth = SeparatorWidth();

	float width = (area.Width() -
		sepWidth * (fSectionCount - 1))
		/ fSectionCount;
	area.left += (index + 1) * width + index * sepWidth;
	area.right = area.left + sepWidth;

	return area;
}


void
SectionEdit::MakeFocus(bool focused)
{
	if (focused == IsFocus())
		return;

	BControl::MakeFocus(focused);

	if (fFocus == -1)
		SectionFocus(0);
	else
		SectionFocus(fFocus);
}


void
SectionEdit::KeyDown(const char* bytes, int32 numbytes)
{
	if (IsEnabled() == false)
		return;
	if (fFocus == -1)
		SectionFocus(0);

	switch (bytes[0]) {
		case B_LEFT_ARROW:
			fFocus -= 1;
			if (fFocus < 0)
				fFocus = fSectionCount - 1;
			SectionFocus(fFocus);
			break;

		case B_RIGHT_ARROW:
			fFocus += 1;
			if ((uint32)fFocus >= fSectionCount)
				fFocus = 0;
			SectionFocus(fFocus);
			break;

		case B_UP_ARROW:
			DoUpPress();
			break;

		case B_DOWN_ARROW:
			DoDownPress();
			break;

		default:
			BControl::KeyDown(bytes, numbytes);
			break;
	}
	Draw(Bounds());
}


status_t
SectionEdit::Invoke(BMessage* message)
{
	if (message == NULL)
		message = Message();
	if (message == NULL)
		return BControl::Invoke(NULL);

	BMessage clone(*message);
	PopulateMessage(&clone);
	return BControl::Invoke(&clone);
}


uint32
SectionEdit::CountSections() const
{
	return fSectionCount;
}


int32
SectionEdit::FocusIndex() const
{
	return fFocus;
}


BRect
SectionEdit::SectionArea() const
{
	BRect sectionArea = Bounds().InsetByCopy(2, 2);
	sectionArea.right -= kArrowAreaWidth;
	return sectionArea;
}


void
SectionEdit::DrawBorder(const BRect& updateRect)
{
	BRect bounds(Bounds());
	bool showFocus = (IsFocus() && Window() && Window()->IsActive());

	be_control_look->DrawTextControlBorder(this, bounds, updateRect, ViewColor(),
		showFocus ? BControlLook::B_FOCUSED : 0);

	SetLowUIColor(B_DOCUMENT_BACKGROUND_COLOR);
	FillRect(bounds, B_SOLID_LOW);

	// draw up/down control

	bounds.left = bounds.right - kArrowAreaWidth;
	bounds.right = Bounds().right - 2;
	fUpRect.Set(bounds.left + 3, bounds.top + 2, bounds.right,
		bounds.bottom / 2.0);
	fDownRect = fUpRect.OffsetByCopy(0, fUpRect.Height() + 2);

	BPoint middle(floorf(fUpRect.left + fUpRect.Width() / 2),
		fUpRect.top + 1);
	BPoint left(fUpRect.left + 3, fUpRect.bottom - 1);
	BPoint right(left.x + 2 * (middle.x - left.x), fUpRect.bottom - 1);

	SetPenSize(2);

	if (updateRect.Intersects(fUpRect)) {
		FillRect(fUpRect, B_SOLID_LOW);
		BeginLineArray(2);
			AddLine(left, middle, HighColor());
			AddLine(middle, right, HighColor());
		EndLineArray();
	}
	if (updateRect.Intersects(fDownRect)) {
		middle.y = fDownRect.bottom - 1;
		left.y = right.y = fDownRect.top + 1;

		FillRect(fDownRect, B_SOLID_LOW);
		BeginLineArray(2);
			AddLine(left, middle, HighColor());
			AddLine(middle, right, HighColor());
		EndLineArray();
	}

	SetPenSize(1);
}


}	// namespace BPrivate
