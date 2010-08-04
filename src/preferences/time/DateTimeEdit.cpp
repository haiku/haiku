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
#include <LocaleRoster.h>
#include <String.h>
#include <Window.h>


using BPrivate::B_LOCAL_TIME;


TTimeEdit::TTimeEdit(BRect frame, const char* name, uint32 sections)
	:
	TSectionEdit(frame, name, sections),
	fLastKeyDownTime(0)
{
	InitView();
	fTime = BDateTime::CurrentDateTime(B_LOCAL_TIME);
}


TTimeEdit::~TTimeEdit()
{
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
		int32 doubleDigi = number + fLastKeyDownInt * 10;
		if (_IsValidDoubleDigi(doubleDigi))
			number = doubleDigi;
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
	SetSections(fSectionArea);
}


void
TTimeEdit::DrawSection(uint32 index, bool hasFocus)
{
	TSection* section = NULL;
	section = static_cast<TSection*> (fSectionList->ItemAt(index));

	if (!section)
		return;

	BRect bounds = section->Frame();
	time_t time = fTime.Time_t();

	SetLowColor(ViewColor());
	BString field;
	if (hasFocus)
		SetLowColor(tint_color(ViewColor(), B_DARKEN_1_TINT));

	BString text;
	int* fieldPositions;
	int fieldCount;

	BLocale locale;
	be_locale_roster->GetDefaultLocale(&locale);
	locale.FormatTime(&text, fieldPositions, fieldCount, time, true);
		// TODO : this should be cached somehow to not redo it for each field

	if (index * 2 + 1 > (uint32)fieldCount) {
		free(fieldPositions);
		return;
	}

	text.CopyCharsInto(field, fieldPositions[index * 2],
		fieldPositions[index * 2 + 1] - fieldPositions[index * 2]);

	free(fieldPositions);

	// calc and center text in section rect
	float width = be_plain_font->StringWidth(field);

	BPoint offset(-((bounds.Width()- width) / 2.0) -1.0,
		bounds.Height() / 2.0 -6.0);

	SetHighColor(0, 0, 0, 255);
	FillRect(bounds, B_SOLID_LOW);
	DrawString(field, bounds.LeftBottom() - offset);
}


void
TTimeEdit::DrawSeparator(uint32 index)
{
	TSection* section = NULL;
	section = static_cast<TSection*> (fSectionList->ItemAt(index));

	if (!section)
		return;

	BRect bounds = section->Frame();
	float sepWidth = SeparatorWidth();

	BString text;
	int* fieldPositions;
	int fieldCount;

	BLocale locale;
	be_locale_roster->GetDefaultLocale(&locale);
	time_t time = fTime.Time_t();
	locale.FormatTime(&text, fieldPositions, fieldCount, time, true);
		// TODO : this should be cached somehow to not redo it for each field

	if (index * 2 + 2 > (uint32)fieldCount) {
		free(fieldPositions);
		return;
	}

	BString field;
	text.CopyCharsInto(field, fieldPositions[index * 2 + 1],
		fieldPositions[index * 2 + 2] - fieldPositions[index * 2 + 1]);

	free(fieldPositions);

	float width = be_plain_font->StringWidth(field);
	BPoint offset(-((sepWidth - width) / 2.0) -1.0, bounds.Height() / 2.0 -6.0);
	DrawString(field, bounds.RightBottom() - offset);
}


void
TTimeEdit::SetSections(BRect area)
{
	// by default divide up the sections evenly
	BRect bounds(area);

	float sepWidth = SeparatorWidth();

	float sep_2 = ceil(sepWidth / fSectionCount +1);
	float width = bounds.Width() / fSectionCount -sep_2;
	bounds.right = bounds.left + (width -sepWidth / fSectionCount);

	for (uint32 idx = 0; idx < fSectionCount; idx++) {
		fSectionList->AddItem(new TSection(bounds));

		bounds.left = bounds.right + sepWidth;
		if (idx == fSectionCount -2)
			bounds.right = area.right -1;
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
	if (fTime.Time().Hour() == hour && fTime.Time().Minute() == minute
		&& fTime.Time().Second() == second)
		return;

	fTime.SetTime(BTime(hour, minute, second));
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
	const char* fields[3] = { "hour", "minute", "second" };

	message->AddBool("time", true);

	BDateElement* dateFormat;
	int fieldCount;
	BLocale here;
	be_locale_roster->GetDefaultLocale(&here);
	here.GetTimeFields(dateFormat, fieldCount, true);
	if (fFocus > fieldCount) {
		free(dateFormat);
		return;
	}

	for (int32 index = 0; index < fSectionList->CountItems() -1; ++index) {
		uint32 data = _SectionValue(index);

		if (fFocus == index)
			data = fHoldValue;

		switch(dateFormat[index]) {
			case B_DATE_ELEMENT_HOUR:
				message->AddInt32(fields[0], data);
				break;
			case B_DATE_ELEMENT_MINUTE:
				message->AddInt32(fields[1], data);
				break;
			case B_DATE_ELEMENT_SECOND:
				message->AddInt32(fields[2], data);
			default:
				break;
		}
	}

	free(dateFormat);
}


void
TTimeEdit::_CheckRange()
{
	int32 value = fHoldValue;
	BDateElement* fields;
	int fieldCount;
	BLocale here;
	be_locale_roster->GetDefaultLocale(&here);
	here.GetTimeFields(fields, fieldCount, true);
	if (fFocus > fieldCount) {
		free(fields);
		return;
	}
	switch (fields[fFocus]) {
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
			free(fields);
			return;
	}

	free(fields);

	fHoldValue = value;
	Invalidate(Bounds());
}


bool
TTimeEdit::_IsValidDoubleDigi(int32 value)
{
	bool isInRange = false;
	BDateElement* fields;
	int fieldCount;
	BLocale here;
	be_locale_roster->GetDefaultLocale(&here);
	here.GetTimeFields(fields, fieldCount, true);
	if (fFocus > fieldCount) {
		free(fields);
		return false;
	}
	switch (fields[fFocus]) {
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
			free(fields);
			return isInRange;
	}

	free(fields);
	return isInRange;
}


int32
TTimeEdit::_SectionValue(int32 index) const
{
	int32 value;
	BDateElement* fields;
	int fieldCount;
	BLocale here;
	be_locale_roster->GetDefaultLocale(&here);
	here.GetTimeFields(fields, fieldCount, true);
	if (index > fieldCount) {
		free(fields);
		return 0;
	}
	switch (fields[index]) {
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

	free(fields);
	return value;
}


//	#pragma mark -


TDateEdit::TDateEdit(BRect frame, const char* name, uint32 sections)
	: TSectionEdit(frame, name, sections)
{
	InitView();
	fDate = BDate::CurrentDate(B_LOCAL_TIME);
}


TDateEdit::~TDateEdit()
{
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
		int32 doubleDigi = number + fLastKeyDownInt * 10;
		if (_IsValidDoubleDigi(doubleDigi))
			number = doubleDigi;
		fLastKeyDownTime = 0;
	} else {
		fLastKeyDownTime = currentTime;
		fLastKeyDownInt = number;
	}

	// if year add 2000

	BDateElement* dateFormat;
	int fieldCount;
	BLocale here;
	be_locale_roster->GetDefaultLocale(&here);
	here.GetDateFields(dateFormat, fieldCount, false);

	if (dateFormat[section] == B_DATE_ELEMENT_YEAR) {
		int32 oldCentury = int32(fHoldValue / 100) * 100;
		if (number < 10 && oldCentury == 1900)
			number += 70;
		number += oldCentury;
	}

	free(dateFormat);

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
	SetSections(fSectionArea);
}


void
TDateEdit::DrawSection(uint32 index, bool hasFocus)
{
	TSection* section = NULL;
	section = static_cast<TSection*> (fSectionList->ItemAt(index));

	if (!section)
		return;

	BRect bounds = section->Frame();
	BDateTime dateTime(fDate, BTime());

	SetLowColor(ViewColor());
	BString field;
	if (hasFocus)
		SetLowColor(tint_color(ViewColor(), B_DARKEN_1_TINT));

	BString text;
	int* fieldPositions;
	int fieldCount;

	BLocale locale;
	be_locale_roster->GetDefaultLocale(&locale);
	locale.FormatDate(&text, fieldPositions, fieldCount, dateTime.Time_t(),
		false);
		// TODO : this should be cached somehow to not redo it for each field

	if (index * 2 + 1 > (uint32)fieldCount) {
		free(fieldPositions);
		return;
	}

	text.CopyCharsInto(field, fieldPositions[index * 2],
		fieldPositions[index * 2 + 1] - fieldPositions[index * 2]);

	free(fieldPositions);


	// calc and center text in section rect
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
	if (index == 3)
		return;

	TSection* section = NULL;
	section = static_cast<TSection*> (fSectionList->ItemAt(index));
	BRect bounds = section->Frame();

	float sepWidth = SeparatorWidth();

	SetHighColor(0, 0, 0, 255);

	BString text;
	int* fieldPositions;
	int fieldCount;

	BLocale locale;
	be_locale_roster->GetDefaultLocale(&locale);
	BDateTime dateTime(fDate, BTime());
	locale.FormatDate(&text, fieldPositions, fieldCount, dateTime.Time_t(),
		false);
		// TODO : this should be cached somehow to not redo it for each field

	if (index * 2 + 2 > (uint32)fieldCount) {
		free(fieldPositions);
		return;
	}

	BString field;
	text.CopyCharsInto(field, fieldPositions[index * 2 + 1],
		fieldPositions[index * 2 + 2] - fieldPositions[index * 2 + 1]);

	free(fieldPositions);

	float width = be_plain_font->StringWidth(field);
	BPoint offset(-((sepWidth - width) / 2.0) -1.0, bounds.Height() / 2.0 -6.0);
	DrawString(field, bounds.RightBottom() - offset);
}


void
TDateEdit::SetSections(BRect area)
{
	// TODO : we have to be more clever here, as the fields can move and have
	// different sizes dependin on the locale

	// create sections
	for (uint32 idx = 0; idx < fSectionCount; idx++)
		fSectionList->AddItem(new TSection(area));

	BRect bounds(area);
	float sepWidth = SeparatorWidth();

	// year
	TSection* section = NULL;
	float width = be_plain_font->StringWidth("0000") +6;
	bounds.right = area.right;
	bounds.left = bounds.right -width;
	section = static_cast<TSection*> (fSectionList->ItemAt(2));
	section->SetFrame(bounds);

	// day
	width = be_plain_font->StringWidth("00") +6;
	bounds.right = bounds.left -sepWidth;
	bounds.left = bounds.right -width;
	section = static_cast<TSection*> (fSectionList->ItemAt(1));
	section->SetFrame(bounds);

	// month
	bounds.right = bounds.left - sepWidth;
	bounds.left = area.left;
	section = static_cast<TSection*> (fSectionList->ItemAt(0));
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
	if (year == fDate.Year() && month == fDate.Month() && day == fDate.Day())
		return;

	fDate.SetDate(year, month, day);
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
	const char* fields[3] = { "month", "day", "year" };

	message->AddBool("time", false);

	BDateElement* dateFormat;
	int fieldCount;
	BLocale here;
	be_locale_roster->GetDefaultLocale(&here);
	here.GetDateFields(dateFormat, fieldCount, false);
	if (fFocus > fieldCount) {
		free(dateFormat);
		return;
	}
	for (int32 index = 0; index < fSectionList->CountItems(); ++index) {
		uint32 data = _SectionValue(index);

		if (fFocus == index)
			data = fHoldValue;

		switch(dateFormat[index]) {
			case B_DATE_ELEMENT_MONTH:
				message->AddInt32(fields[0], data);
				break;
			case B_DATE_ELEMENT_DAY:
				message->AddInt32(fields[1], data);
				break;
			case B_DATE_ELEMENT_YEAR:
				message->AddInt32(fields[2], data);
				break;
			default:
				break;
		}
	}

	free(dateFormat);
}


void
TDateEdit::_CheckRange()
{
	int32 value = fHoldValue;
	BDateElement* fields;
	int fieldCount;
	BLocale here;
	be_locale_roster->GetDefaultLocale(&here);
	here.GetDateFields(fields, fieldCount, false);
	if (fFocus > fieldCount) {
		free(fields);
		return;
	}

	switch (fields[fFocus]) {
		case B_DATE_ELEMENT_MONTH:
			if (value > 12)
				value = 1;
			else if (value < 1)
				value = 12;

			fDate.SetDate(fDate.Year(), value, fDate.Day());
			break;

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

		case B_DATE_ELEMENT_YEAR:
			// 2037 is the end of 32-bit UNIX time
			if (value > 2037)
				value = 2037;
			else if (value < 1970)
				value = 1970;

			fDate.SetDate(value, fDate.Month(), fDate.Day());
			break;

		default:
			free(fields);
			return;
	}

	free(fields);
	fHoldValue = value;
	Draw(Bounds());
}


bool
TDateEdit::_IsValidDoubleDigi(int32 value)
{
	bool isInRange = false;
	BDateElement* fields;
	int fieldCount;
	BLocale here;
	be_locale_roster->GetDefaultLocale(&here);
	here.GetDateFields(fields, fieldCount, false);
	if (fFocus > fieldCount) {
		free(fields);
		return false;
	}
	int32 year = 0;
	switch (fields[fFocus]) {
		case B_DATE_ELEMENT_DAY:
		{
			int32 days = fDate.DaysInMonth();
			if (value <= days)
				isInRange = true;
			break;
		}

		case B_DATE_ELEMENT_YEAR:
		{
			year = int32(fHoldValue / 100) * 100 + value;
			if (year <= 2037 && year >= 1970)
				isInRange = true;
			break;
		}

		default:
			free(fields);
			return isInRange;
	}

	free(fields);
	return isInRange;
}


int32
TDateEdit::_SectionValue(int32 index) const
{
	int32 value = 0;
	BDateElement* fields;
	int fieldCount;
	BLocale here;
	be_locale_roster->GetDefaultLocale(&here);
	here.GetDateFields(fields, fieldCount, false);
	if (index > fieldCount) {
		free(fields);
		return 0;
	}
	switch (fields[index]) {
		case B_DATE_ELEMENT_MONTH:
			value = fDate.Month();
			break;

		case B_DATE_ELEMENT_DAY:
			value = fDate.Day();
			break;

		default:
			value = fDate.Year();
			break;
	}

	return value;
}
