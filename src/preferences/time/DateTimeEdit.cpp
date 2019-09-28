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

#include <DateFormat.h>
#include <List.h>
#include <Locale.h>
#include <String.h>
#include <Window.h>


using BPrivate::B_LOCAL_TIME;


TTimeEdit::TTimeEdit(const char* name, uint32 sections)
	:
	TSectionEdit(name, sections),
	fLastKeyDownTime(0),
	fFields(NULL),
	fFieldCount(0),
	fFieldPositions(NULL),
	fFieldPosCount(0)
{
	InitView();
}


TTimeEdit::~TTimeEdit()
{
	free(fFieldPositions);
	free(fFields);
}


void
TTimeEdit::KeyDown(const char* bytes, int32 numBytes)
{
	TSectionEdit::KeyDown(bytes, numBytes);

	// only accept valid input
	int32 number = atoi(bytes);
	if (number - 1 < 0)
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

	// send message to change time
	DispatchMessage();
}


void
TTimeEdit::InitView()
{
	// make sure we call the base class method, as it
	// will create the arrow bitmaps and the section list
	fTime = BDateTime::CurrentDateTime(B_LOCAL_TIME);
	_UpdateFields();
}


void
TTimeEdit::DrawSection(uint32 index, BRect bounds, bool hasFocus)
{
	if (fFieldPositions == NULL || index * 2 + 1 >= (uint32)fFieldPosCount)
		return;

	if (hasFocus)
		SetLowColor(mix_color(ui_color(B_CONTROL_HIGHLIGHT_COLOR),
			ViewColor(), 192));
	else
		SetLowColor(ViewColor());

	BString field;
	fText.CopyCharsInto(field, fFieldPositions[index * 2],
		fFieldPositions[index * 2 + 1] - fFieldPositions[index * 2]);

	BPoint point(bounds.LeftBottom());
	point.y -= bounds.Height() / 2.0 - 6.0;
	point.x += (bounds.Width() - StringWidth(field)) / 2;
	SetHighUIColor(B_PANEL_TEXT_COLOR);
	FillRect(bounds, B_SOLID_LOW);
	DrawString(field, point);
}


void
TTimeEdit::DrawSeparator(uint32 index, BRect bounds)
{
	if (fFieldPositions == NULL || index * 2 + 2 >= (uint32)fFieldPosCount)
		return;

	BString field;
	fText.CopyCharsInto(field, fFieldPositions[index * 2 + 1],
		fFieldPositions[index * 2 + 2] - fFieldPositions[index * 2 + 1]);

	BPoint point(bounds.LeftBottom());
	point.y -= bounds.Height() / 2.0 - 6.0;
	point.x += (bounds.Width() - StringWidth(field)) / 2;
	SetHighUIColor(B_PANEL_TEXT_COLOR);
	DrawString(field, point);
}


float
TTimeEdit::SeparatorWidth()
{
	return 10.0f;
}


float
TTimeEdit::MinSectionWidth()
{
	return be_plain_font->StringWidth("00");
}


void
TTimeEdit::SectionFocus(uint32 index)
{
	fLastKeyDownTime = 0;
	fFocus = index;
	fHoldValue = _SectionValue(index);
	Draw(Bounds());
}


void
TTimeEdit::SetTime(int32 hour, int32 minute, int32 second)
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


void
TTimeEdit::DoUpPress()
{
	if (fFocus == -1)
		SectionFocus(0);

	// update displayed value
	fHoldValue += 1;

	_CheckRange();

	// send message to change time
	DispatchMessage();
}


void
TTimeEdit::DoDownPress()
{
	if (fFocus == -1)
		SectionFocus(0);

	// update display value
	fHoldValue -= 1;

	_CheckRange();

	// send message to change time
	DispatchMessage();
	
}


void
TTimeEdit::BuildDispatch(BMessage* message)
{
	if (fFocus < 0 || fFocus >= fFieldCount)
		return;

	message->AddBool("time", true);

	for (int32 index = 0; index < (int)fSectionCount; ++index) {
		uint32 data = _SectionValue(index);

		if (fFocus == index)
			data = fHoldValue;

		switch (fFields[index]) {
			case B_DATE_ELEMENT_HOUR:
				message->AddInt32("hour", data);
				break;

			case B_DATE_ELEMENT_MINUTE:
				message->AddInt32("minute", data);
				break;

			case B_DATE_ELEMENT_SECOND:
				message->AddInt32("second", data);
				break;

			default:
				break;
		}
	}
}


void
TTimeEdit::_UpdateFields()
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
TTimeEdit::_CheckRange()
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
TTimeEdit::_IsValidDoubleDigit(int32 value)
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
TTimeEdit::_SectionValue(int32 index) const
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
TTimeEdit::PreferredHeight()
{
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	return ceilf((fontHeight.ascent + fontHeight.descent) * 1.4);
}


//	#pragma mark -


TDateEdit::TDateEdit(const char* name, uint32 sections)
	:
	TSectionEdit(name, sections),
	fFields(NULL),
	fFieldCount(0),
	fFieldPositions(NULL),
	fFieldPosCount(0)
{
	InitView();
}


TDateEdit::~TDateEdit()
{
	free(fFieldPositions);
	free(fFields);
}


void
TDateEdit::KeyDown(const char* bytes, int32 numBytes)
{
	TSectionEdit::KeyDown(bytes, numBytes);

	// only accept valid input
	int32 number = atoi(bytes);
	if (number - 1 < 0)
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

	// update display value
	fHoldValue = number;

	_CheckRange();

	// send message to change time
	DispatchMessage();
}


void
TDateEdit::InitView()
{
	// make sure we call the base class method, as it
	// will create the arrow bitmaps and the section list
	fDate = BDate::CurrentDate(B_LOCAL_TIME);
	_UpdateFields();
}


void
TDateEdit::DrawSection(uint32 index, BRect bounds, bool hasFocus)
{
	if (fFieldPositions == NULL || index * 2 + 1 >= (uint32)fFieldPosCount)
		return;

	if (hasFocus)
		SetLowColor(mix_color(ui_color(B_CONTROL_HIGHLIGHT_COLOR),
			ViewColor(), 192));
	else
		SetLowColor(ViewColor());

	BString field;
	fText.CopyCharsInto(field, fFieldPositions[index * 2],
		fFieldPositions[index * 2 + 1] - fFieldPositions[index * 2]);

	BPoint point(bounds.LeftBottom());
	point.y -= bounds.Height() / 2.0 - 6.0;
	point.x += (bounds.Width() - StringWidth(field)) / 2;
	SetHighUIColor(B_PANEL_TEXT_COLOR);
	FillRect(bounds, B_SOLID_LOW);
	DrawString(field, point);
}


void
TDateEdit::DrawSeparator(uint32 index, BRect bounds)
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
	SetHighUIColor(B_PANEL_TEXT_COLOR);
	DrawString(field, point);
}


void
TDateEdit::SectionFocus(uint32 index)
{
	fLastKeyDownTime = 0;
	fFocus = index;
	fHoldValue = _SectionValue(index);
	Draw(Bounds());
}


float
TDateEdit::MinSectionWidth()
{
	return be_plain_font->StringWidth("00");
}


float
TDateEdit::SeparatorWidth()
{
	return 10.0f;
}


void
TDateEdit::SetDate(int32 year, int32 month, int32 day)
{
	fDate.SetDate(year, month, day);

	if (LockLooper()) {
		_UpdateFields();
		UnlockLooper();
	}

	Invalidate(Bounds());
}


void
TDateEdit::DoUpPress()
{
	if (fFocus == -1)
		SectionFocus(0);

	// update displayed value
	fHoldValue += 1;

	_CheckRange();

	// send message to change Date
	DispatchMessage();
}


void
TDateEdit::DoDownPress()
{
	if (fFocus == -1)
		SectionFocus(0);

	// update display value
	fHoldValue -= 1;

	_CheckRange();

	// send message to change Date
	DispatchMessage();
}


void
TDateEdit::BuildDispatch(BMessage* message)
{
	if (fFocus < 0 || fFocus >= fFieldCount)
		return;

	message->AddBool("time", false);

	for (int32 index = 0; index < (int)fSectionCount; ++index) {
		uint32 data = _SectionValue(index);

		if (fFocus == index)
			data = fHoldValue;

		switch (fFields[index]) {
			case B_DATE_ELEMENT_MONTH:
				message->AddInt32("month", data);
				break;

			case B_DATE_ELEMENT_DAY:
				message->AddInt32("day", data);
				break;

			case B_DATE_ELEMENT_YEAR:
				message->AddInt32("year", data);
				break;

			default:
				break;
		}
	}
}


void
TDateEdit::_UpdateFields()
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
TDateEdit::_CheckRange()
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
			if (value > 12)
				value = 1;
			else if (value < 1)
				value = 12;

			fDate.SetDate(fDate.Year(), value, fDate.Day());
			break;

		case B_DATE_ELEMENT_YEAR:
			fDate.SetDate(value, fDate.Month(), fDate.Day());
			break;

		default:
			return;
	}

	fHoldValue = value;
	Draw(Bounds());
}


bool
TDateEdit::_IsValidDoubleDigit(int32 value)
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
TDateEdit::_SectionValue(int32 index) const
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
TDateEdit::PreferredHeight()
{
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	return ceilf((fontHeight.ascent + fontHeight.descent) * 1.4);
}

