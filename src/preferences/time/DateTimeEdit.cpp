/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		probably Mike Berg <mike@agamemnon.homelinux.net>
 *		and/or Andrew McCall <mccall@@digitalparadise.co.uk>
 *		Julun <host.haiku@gmx.de>
 *
 */

#include "DateTimeEdit.h"
#include "DateUtils.h"


#include <List.h>
#include <String.h>


#define YEAR_DELTA_MAX 110
#define YEAR_DELTA_MIN 64


class TDateTimeSection : public TSection {
	public:
				TDateTimeSection(BRect frame, uint32 data = 0)
					: TSection(frame), fData(data) { }
				~TDateTimeSection();
		
		uint32	Data() const	{	return fData;	}
		void	SetData(uint32 data)	{	fData = data;	}

	private:
		uint32 fData;
};


//	#pragma mark -


TTimeEdit::TTimeEdit(BRect frame, const char *name, uint32 sections)
	: TSectionEdit(frame, name, sections)
{
	InitView();
}


TTimeEdit::~TTimeEdit()
{
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
	// user defined section drawing
	TDateTimeSection *section;
	section = (TDateTimeSection *)fSectionList->ItemAt(index);

	BRect bounds = section->Frame();

	// format value to display

	uint32 value;

	if (hasFocus) {
		SetLowColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
		value = fHoldValue;
	} else {
		SetLowColor(ViewColor());
		value = section->Data();
	}

	BString text;
	// format value (new method?)
	switch (index) {
		case 0: // hour
			if (value > 12) {
				if (value < 22)
					text << "0";
				text << value - 12;
			} else if (value == 0) {
				text << "12";
			} else {
				if (value < 10)
					text << "0";
				text << value;
			}			
		break;

		case 1: // minute
		case 2: // second
			if (value < 10)
				text << "0";
			text << value;
		break;

		case 3: // am/pm
			value = ((TDateTimeSection *)fSectionList->ItemAt(0))->Data();
			if (value >= 12)
				text << "PM";
			else 
				text << "AM";
		break;
		
		default:
			return;
		break;
	}

	// calc and center text in section rect
	float width = be_plain_font->StringWidth(text.String());
	
	BPoint offset(-(bounds.Width() / 2.0 -width / 2.0) -1.0, bounds.Height() / 2.0 -6.0);
	
	BPoint drawpt(bounds.LeftBottom() -offset);
	
	SetHighColor(0, 0, 0, 255);
	FillRect(bounds, B_SOLID_LOW);
	DrawString(text.String(), drawpt);	
}


/* 
	DrawSeperator(uint32 index) user drawn seperator. Section is the
	index of the section in fSectionList or the section to the seps left
*/
void
TTimeEdit::DrawSeperator(uint32 index)
{
	if (index == 3)
		return;  

	TDateTimeSection *section = (TDateTimeSection *)fSectionList->ItemAt(index);
	
	BRect bounds = section->Frame();
	float sepWidth = SeperatorWidth();	

	char* sep = ":";
	if (index == 2)
		sep = "-";

	float width = be_plain_font->StringWidth(sep);
	
	BPoint offset(-(sepWidth / 2.0 - width / 2.0) -1.0, bounds.Height() / 2.0 -6.0);
	BPoint drawpt(bounds.RightBottom() -offset);

	DrawString(sep, drawpt);	
}


void
TTimeEdit::SetSections(BRect area)
{
	// by default divie up the sections evenly
	BRect bounds(area);
	// no comp for sep width	
	
	float sepWidth = SeperatorWidth();
	
	float sep_2 = ceil(sepWidth / fSectionCount +1);
	float width = bounds.Width() / fSectionCount -sep_2;
	bounds.right = bounds.left + (width -sepWidth / fSectionCount);
		
	for (uint32 idx = 0; idx < fSectionCount; idx++) {
		fSectionList->AddItem(new TDateTimeSection(bounds));
		
		bounds.left = bounds.right + sepWidth;
		if (idx == fSectionCount -2)
			bounds.right = area.right -1;
		else
			bounds.right = bounds.left + (width -sep_2);
	}
}


float
TTimeEdit::SeperatorWidth() const
{
	return 8.0f;
}


void
TTimeEdit::SectionFocus(uint32 index)
{
	fFocus = index;

	// update hold value
	fHoldValue = ((TDateTimeSection *)fSectionList->ItemAt(fFocus))->Data();
	
	Draw(Bounds());
}


void
TTimeEdit::SetTime(uint32 hour, uint32 minute, uint32 second)
{
	if (fSectionList->CountItems()> 0)
	{
		bool update = false;
		
		TDateTimeSection *section = (TDateTimeSection *)fSectionList->ItemAt(0);
		if (section->Data() != hour) {
			section->SetData(hour);
			update = true;
		}
		
		section = (TDateTimeSection *)fSectionList->ItemAt(1);
		if (section->Data() != minute) {
			section->SetData(minute);
			update = true;
		}
			
		section = (TDateTimeSection *)fSectionList->ItemAt(2);
		if (section->Data() != second) {
			section->SetData(second);
			update = true;
		}
		
		if (update)
			Draw(Bounds());
	}
}


void
TTimeEdit::DoUpPress()
{
	if (fFocus == -1)
		SectionFocus(0);
	
	// update displayed value
	fHoldValue += 1;
	
	CheckRange();
	
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
	
	CheckRange();
	
	// send message to change time
	DispatchMessage();
}
		
		
void
TTimeEdit::BuildDispatch(BMessage *message)
{
	const char *fields[4] = {"hour", "minute", "second", "isam"};
	
	message->AddBool("time", true);
	
	for (int32 idx = 0; idx < fSectionList->CountItems() -1; idx++) {
		uint32 data = ((TDateTimeSection *)fSectionList->ItemAt(idx))->Data();
		
		if (fFocus == idx)
			data = fHoldValue;
		
		if (idx == 3) // isam
			message->AddBool(fields[idx], data == 1);
		else
			message->AddInt32(fields[idx], data);
	}
}


void
TTimeEdit::CheckRange()
{
	int32 value = fHoldValue;
	switch (fFocus) {
		case 0: // hour
			if (value> 23) 
				value = 0;
			else if (value < 0) 
				value = 23;
			break;

		case 1: // minute
			if (value> 59)
				value = 0;
			else if (value < 0)
				value = 59;
			break;

		case 2: // second
			if (value > 59)
				value = 0;
			else if (value < 0)
				value = 59;
			
			break;

		case 3:
			// modify hour value to reflect change in am/pm
			value = ((TDateTimeSection *)fSectionList->ItemAt(0))->Data();
			if (value < 13)
				value += 12;
			else
				value -= 12;
			if (value == 24)
				value = 0;
			((TDateTimeSection *)fSectionList->ItemAt(0))->SetData(value);
			break;

		default:
			return;
	}

	((TDateTimeSection *)fSectionList->ItemAt(fFocus))->SetData(value);
	fHoldValue = value;

	Invalidate(Bounds());
}


//	#pragma mark -


TDateEdit::TDateEdit(BRect frame, const char *name, uint32 sections)
	: TSectionEdit(frame, name, sections)
{
	InitView();
}


TDateEdit::~TDateEdit()
{
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
	// user defined section drawing
	TDateTimeSection *section;
	section = (TDateTimeSection *)fSectionList->ItemAt(index);

	BRect bounds = section->Frame();

	// format value to display

	uint32 value;

	if (hasFocus) {
		SetLowColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
		value = fHoldValue;
	} else {
		SetLowColor(ViewColor());
		value = section->Data();
	}

	BString text;
	switch (index) {
		case 0:
		{	 // month
			struct tm tm;
			tm.tm_mon = value;

			char buffer[64];
			memset(buffer, 0, sizeof(buffer));
			strftime(buffer, sizeof(buffer), "%B", &tm);
			text.SetTo(buffer);
		}	break;

		case 1: // day
			text << value;
			break;

		case 2: // year
			text << (value + 1900);
			break;

		default:
			return;
	}

	// calc and center text in section rect
	float width = StringWidth(text.String());
	BPoint offset(-(bounds.Width() - width) / 2.0 - 1.0, (bounds.Height() / 2.0 - 6.0));

	if (index == 0)
		offset.x = -(bounds.Width() - width) ;

	SetHighColor(0, 0, 0, 255);
	FillRect(bounds, B_SOLID_LOW);
	DrawString(text.String(), bounds.LeftBottom() - offset);	
}


/*
	DrawSeperator(uint32 index) user drawn seperator. Section is the
	index of the section in fSectionList or the section to the seps left
*/
void
TDateEdit::DrawSeperator(uint32 index)
{
	if (index == 3)
		return;

	TDateTimeSection *section = (TDateTimeSection *)fSectionList->ItemAt(index);
	BRect bounds = section->Frame();

	float sepWidth = SeperatorWidth();
	float width = be_plain_font->StringWidth("/");

	BPoint offset(-(sepWidth / 2.0 - width / 2.0) -1.0, bounds.Height() / 2.0 -6.0);
	BPoint drawpt(bounds.RightBottom() - offset);

	DrawString("/", drawpt);	
}


void
TDateEdit::SetSections(BRect area)
{
	// create sections
	for (uint32 idx = 0; idx < fSectionCount; idx++)
		fSectionList->AddItem(new TDateTimeSection(area));

	BRect bounds(area);

	// year
	float width = be_plain_font->StringWidth("0000") +6;
	bounds.right = area.right;
	bounds.left = bounds.right -width;
	((TDateTimeSection *)fSectionList->ItemAt(2))->SetFrame(bounds);

	float sepWidth = SeperatorWidth();

	// day
	width = be_plain_font->StringWidth("00") +6;
	bounds.right = bounds.left -sepWidth;
	bounds.left = bounds.right -width;
	((TDateTimeSection *)fSectionList->ItemAt(1))->SetFrame(bounds);

	// month
	bounds.right = bounds.left - sepWidth;
	bounds.left = area.left;
	((TDateTimeSection *)fSectionList->ItemAt(0))->SetFrame(bounds);
}


float
TDateEdit::SeperatorWidth() const
{
	return 8.0f;
}


void
TDateEdit::SectionFocus(uint32 index)
{
	fFocus = index;

	// update hold value
	fHoldValue = ((TDateTimeSection *)fSectionList->ItemAt(fFocus))->Data();

	Draw(Bounds());
}


void
TDateEdit::SetDate(uint32 year, uint32 month, uint32 day)
{
	if (fSectionList->CountItems() > 0) {
		bool update = false;

		TDateTimeSection *section = (TDateTimeSection *)fSectionList->ItemAt(0);
		if (section->Data() != month) {
			section->SetData(month);
			update = true;
		}

		section = (TDateTimeSection *)fSectionList->ItemAt(1);
		if (section->Data() != day) {
			section->SetData(day);
			update = true;
		}

		section = (TDateTimeSection *)fSectionList->ItemAt(2);
		if (section->Data() != year) {
			section->SetData(year);
			update = true;
		}

		if (update)
			Invalidate(Bounds());
	}
}


void
TDateEdit::DoUpPress()
{
	if (fFocus == -1)
		SectionFocus(0);

	// update displayed value
	fHoldValue += 1;

	CheckRange();

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

	CheckRange();

	// send message to change Date
	DispatchMessage();
}
		
		
void
TDateEdit::BuildDispatch(BMessage *message)
{
	const char *fields[3] = {"month", "day", "year"};

	message->AddBool("time", false);

	for (int32 idx = 0; idx < fSectionList->CountItems(); idx++) {
		uint32 data = ((TDateTimeSection *)fSectionList->ItemAt(idx))->Data();

		if (fFocus == idx)
			data = fHoldValue;

		message->AddInt32(fields[idx], data);
	}
}


void
TDateEdit::CheckRange()
{
	int32 value = fHoldValue;
	
	switch (fFocus) {
		case 0: // month
		{
			if (value > 11)
				value = 0;
			else if (value < 0)
				value = 11;
			break;
		}

		case 1: //day
		{
			uint32 month = ((TDateTimeSection *)fSectionList->ItemAt(0))->Data();
			uint32 year = ((TDateTimeSection *)fSectionList->ItemAt(2))->Data();
			
			int daycnt = getDaysInMonth(month, year);
			if (value > daycnt)
				value = 1;
			else if (value < 1)
				value = daycnt;
			break;
		}

		case 2: //year
		{
			if (value > YEAR_DELTA_MAX)
				value = YEAR_DELTA_MIN;
			else if (value < YEAR_DELTA_MIN)
				value = YEAR_DELTA_MAX;
			break;
		}

		default:
			return;
	}

	((TDateTimeSection *)fSectionList->ItemAt(fFocus))->SetData(value);
	fHoldValue = value;

	Draw(Bounds());
}

