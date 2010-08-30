/*
 * Copyright 2004-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		McCall <mccall@@digitalparadise.co.uk>
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 *		Clemens <mail@Clemens-Zeidler.de>
 *		Adrien Destugues <pulkomandy@pulkomandy.cx>
 */


#include "DateTimeEdit.h"

#include <stdlib.h>

#include <List.h>
#include <Locale.h>
#include <String.h>
#include <Window.h>


using BPrivate::B_LOCAL_TIME;


TTimeEdit::TTimeEdit(BRect frame, const char* name, uint32 sections)
	:
	TSectionEdit(frame, name, sections),
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
	TSectionEdit::InitView();

	fTime = BDateTime::CurrentDateTime(B_LOCAL_TIME);
	_UpdateFields();

	SetSections(fSectionArea);
}


void
TTimeEdit::DrawSection(uint32 index, bool hasFocus)
{
	TSection* section = static_cast<TSection*>(fSectionList->ItemAt(index));
	if (!section)
		return;

	if (fFieldPositions == NULL || index * 2 + 1 > (uint32)fFieldPosCount)
		return;

	BRect bounds = section->Frame();

	SetLowColor(ViewColor());
	if (hasFocus)
		SetLowColor(tint_color(ViewColor(), B_DARKEN_1_TINT));

	BString field;
	fText.CopyCharsInto(field, fFieldPositions[index * 2],
		fFieldPositions[index * 2 + 1] - fFieldPositions[index * 2]);

	// calc and center text in section rect
	float width = be_plain_font->StringWidth(field);

	BPoint offset(-((bounds.Width()- width) / 2.0) - 1.0,
		bounds.Height() / 2.0 - 6.0);

	SetHighColor(0, 0, 0, 255);
	FillRect(bounds, B_SOLID_LOW);
	DrawString(field, bounds.LeftBottom() - offset);
}


void
TTimeEdit::DrawSeparator(uint32 index)
{
	TSection* section = static_cast<TSection*>(fSectionList->ItemAt(index));
	if (!section)
		return;

	if (fFieldPositions == NULL || index * 2 + 2 > (uint32)fFieldPosCount)
		return;

	BString field;
	fText.CopyCharsInto(field, fFieldPositions[index * 2 + 1],
		fFieldPositions[index * 2 + 2] - fFieldPositions[index * 2 + 1]);

	float sepWidth = SeparatorWidth();
	BRect bounds = section->Frame();
	float width = be_plain_font->StringWidth(field);
	BPoint offset(-((sepWidth - width) / 2.0) - 1.0,
		bounds.Height() / 2.0 - 6.0);
	DrawString(field, bounds.RightBottom() - offset);
}


void
TTimeEdit::SetSections(BRect area)
{
	// by default divide up the sections evenly
	BRect bounds(area);

	float sepWidth = SeparatorWidth();

	float sep_2 = ceil(sepWidth / fSectionCount + 1);
	float width = bounds.Width() / fSectionCount - sep_2;
	bounds.right = bounds.left + (width - sepWidth / fSectionCount);

	for (uint32 idx = 0; idx < fSectionCount; idx++) {
		fSectionList->AddItem(new TSection(bounds));

		bounds.left = bounds.right + sepWidth;
		if (idx == fSectionCount - 2)
			bounds.right = area.right - 1;
		else
			bounds.right = bounds.left + (width - sep_2);
	}
}


float
TTimeEdit::SeparatorWidth() const
{
	return 10.0f;
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

	for (int32 index = 0; index < fSectionList->CountItems() - 1; ++index) {
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
	be_locale->FormatTime(&fText, fFieldPositions, fFieldPosCount, time, true);
	be_locale->GetTimeFields(fFields, fFieldCount, true);
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


//	#pragma mark -


TDateEdit::TDateEdit(BRect frame, const char* name, uint32 sections)
	:
	TSectionEdit(frame, name, sections),
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
	TSectionEdit::InitView();

	fDate = BDate::CurrentDate(B_LOCAL_TIME);
	_UpdateFields();

	SetSections(fSectionArea);
}


void
TDateEdit::DrawSection(uint32 index, bool hasFocus)
{
	TSection* section = static_cast<TSection*>(fSectionList->ItemAt(index));
	if (!section)
		return;

	if (fFieldPositions == NULL || index * 2 + 1 > (uint32)fFieldPosCount)
		return;

	SetLowColor(ViewColor());
	if (hasFocus)
		SetLowColor(tint_color(ViewColor(), B_DARKEN_1_TINT));

	BString field;
	fText.CopyCharsInto(field, fFieldPositions[index * 2],
		fFieldPositions[index * 2 + 1] - fFieldPositions[index * 2]);

	// calc and center text in section rect
	BRect bounds = section->Frame();
	float width = StringWidth(field);
	BPoint offset(-(bounds.Width() - width) / 2.0 - 1.0,
		(bounds.Height() / 2.0 - 6.0));

	SetHighColor(0, 0, 0, 255);
	FillRect(bounds, B_SOLID_LOW);
	DrawString(field, bounds.LeftBottom() - offset);
}


void
TDateEdit::DrawSeparator(uint32 index)
{
	if (index < 0 || index >= 2)
		return;

	TSection* section = static_cast<TSection*>(fSectionList->ItemAt(index));
	if (!section)
		return;

	if (fFieldPositions == NULL || index * 2 + 2 > (uint32)fFieldPosCount)
		return;

	BString field;
	fText.CopyCharsInto(field, fFieldPositions[index * 2 + 1],
		fFieldPositions[index * 2 + 2] - fFieldPositions[index * 2 + 1]);

	BRect bounds = section->Frame();
	float width = be_plain_font->StringWidth(field);
	float sepWidth = SeparatorWidth();
	BPoint offset(-((sepWidth - width) / 2.0) - 1.0,
		bounds.Height() / 2.0 - 6.0);

	SetHighColor(0, 0, 0, 255);
	DrawString(field, bounds.RightBottom() - offset);
}


void
TDateEdit::SetSections(BRect area)
{
	// TODO : we have to be more clever here, as the fields can move and have
	// different sizes depending on the locale

	// create sections
	for (uint32 idx = 0; idx < fSectionCount; idx++)
		fSectionList->AddItem(new TSection(area));

	BRect bounds(area);
	float sepWidth = SeparatorWidth();

	TSection* section = static_cast<TSection*>(fSectionList->ItemAt(0));
	float width = be_plain_font->StringWidth("0000") + 10;
	bounds.left = area.left;
	bounds.right = bounds.left + width;
	section->SetFrame(bounds);

	section = static_cast<TSection*>(fSectionList->ItemAt(1));
	width = be_plain_font->StringWidth("0000") + 10;
	bounds.left = bounds.right + sepWidth;
	bounds.right = bounds.left + width;
	section->SetFrame(bounds);

	section = static_cast<TSection*>(fSectionList->ItemAt(2));
	width = be_plain_font->StringWidth("0000") + 10;
	bounds.left = bounds.right + sepWidth;
	bounds.right = bounds.left + width;
	section->SetFrame(bounds);
}


float
TDateEdit::SeparatorWidth() const
{
	return 10.0f;
}


void
TDateEdit::SectionFocus(uint32 index)
{
	fLastKeyDownTime = 0;
	fFocus = index;
	fHoldValue = _SectionValue(index);
	Draw(Bounds());
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

	for (int32 index = 0; index < fSectionList->CountItems(); ++index) {
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
	be_locale->FormatDate(&fText, fFieldPositions, fFieldPosCount, time, false);
	be_locale->GetDateFields(fFields, fFieldCount, false);
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
			// 2037 is the end of 32-bit UNIX time
			if (value > 2037)
				value = 2037;
			else if (value < 1970)
				value = 1970;

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
			if (year <= 2037 && year >= 1970)
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
