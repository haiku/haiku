#include "DateTimeEdit.h"
#include "DateUtils.h"

#include <String.h>
#include <stdio.h>

#define YEAR_DELTA_MAX 110
#define YEAR_DELTA_MIN 64


TDateTimeSection::TDateTimeSection(BRect frame, uint32 data = 0)
	: TSection(frame)
	, f_data(data)
{
}


TDateTimeSection::~TDateTimeSection()
{
}


uint32
TDateTimeSection::Data() const
{
	return f_data;
}


void
TDateTimeSection::SetData(uint32 data)
{
	f_data = data;
}




TTimeEdit::TTimeEdit(BRect frame, const char *name, uint32 sections)
	: TSectionEdit(frame, name, sections)
{
	InitView();
}


TTimeEdit::~TTimeEdit()
{
	TSection *section;
	if (f_sections->CountItems() > 0)
		for (int32 idx = 0; (section = (TSection *)f_sections->ItemAt(idx)); idx++)
			delete section;
	delete f_sections;
}


void
TTimeEdit::InitView()
{
	TSectionEdit::InitView();
	SetSections(f_sectionsarea);
}


void
TTimeEdit::DrawSection(uint32 index, bool isfocus)
{
	if (f_sections->CountItems() == 0)
		printf("No Sections!!!\n");
		
	// user defined section drawing
	TDateTimeSection *section;
	section = (TDateTimeSection *)f_sections->ItemAt(index);
	
	BRect bounds = section->Frame();
	
	
	// format value to display

	uint32 value;
	
	if (isfocus) {
		SetLowColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
		value = f_holdvalue;
	} else {
		SetLowColor(ViewColor());
		value = section->Data();
	}
	
	BString text;
	// format value (new method?)
	switch (index) {
		case 0: // hour
			if (value> 12) {
				if (value < 22) text << "0";
				text << value -12;
			} else if (value == 0) {
				text << "12";
			} else {
				if (value <10) text << "0";
				text << value;
			}			
		break;
		
		case 1: // minute
		case 2: // second
			if (value <10) text << "0";
			text << value;
		break;

		case 3: //am/pm
			value = ((TDateTimeSection *)f_sections->ItemAt(0))->Data();
			if (value >12 || value == 0)
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
	
	BPoint offset(-(bounds.Width()/2.0 -width/2.0) -1.0, bounds.Height()/2.0 -6.0);
	
	BPoint drawpt(bounds.LeftBottom() -offset);
	
	SetHighColor(0, 0, 0, 255);
	FillRect(bounds, B_SOLID_LOW);
	DrawString(text.String(), drawpt);	
}


void
TTimeEdit::DrawSeperator(uint32 index)
{
	/* user drawn seperator.  section is the index of the section 
	in f_sections or the section to the seps left
	*/
	
	if (index == 3) return;  // no seperator for am/pm
	BString text(":");
	uint32 sep;
	GetSeperatorWidth(&sep);
	
	TDateTimeSection *section = (TDateTimeSection *)f_sections->ItemAt(index);
	BRect bounds = section->Frame();
	
	float width = be_plain_font->StringWidth(text.String());
	BPoint offset(-(sep/2.0 -width/2.0) -1.0, bounds.Height()/2.0 -6.0);
	BPoint drawpt(bounds.RightBottom() -offset);

	DrawString(text.String(), drawpt);	
}


void
TTimeEdit::SetSections(BRect area)
{
	// by default divie up the sections evenly
	BRect bounds(area);
	// no comp for sep width	
	
	uint32 sep_width;
	GetSeperatorWidth(&sep_width);
	
	float sep_2 = ceil(sep_width/f_sectioncount +1);
	float width = bounds.Width()/f_sectioncount -sep_2;
	bounds.right = bounds.left +(width -sep_width/f_sectioncount);
		
	TDateTimeSection *section;

	for (uint32 idx = 0; idx < f_sectioncount; idx++) {
		section = new TDateTimeSection(bounds);
		f_sections->AddItem(section);
		
		bounds.left = bounds.right +sep_width;
		if (idx == f_sectioncount -2)
			bounds.right = area.right -1;
		else
			bounds.right = bounds.left +(width -sep_2);
	}
}


void
TTimeEdit::GetSeperatorWidth(uint32 *width)
{
	*width = 8;
}


void
TTimeEdit::SectionFocus(uint32 index)
{
	f_focus = index;

	// update hold value
	f_holdvalue = ((TDateTimeSection *)f_sections->ItemAt(f_focus))->Data();
	
	Draw(Bounds());
}


void
TTimeEdit::SetTo(uint32 hour, uint32 minute, uint32 second)
{
	if (f_sections->CountItems()> 0)
	{
		bool update = false;
		
		TDateTimeSection *section = (TDateTimeSection *)f_sections->ItemAt(0);
		if (section->Data() != hour) {
			section->SetData(hour);
			update = true;
		}
		
		section = (TDateTimeSection *)f_sections->ItemAt(1);
		if (section->Data() != minute) {
			section->SetData(minute);
			update = true;
		}
			
		section = (TDateTimeSection *)f_sections->ItemAt(2);
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
	// update displayed value
	f_holdvalue += 1;
	
	CheckRange();
	
	// send message to change time
	DispatchMessage();
}

		
void
TTimeEdit::DoDownPress()
{
	// update display value
	f_holdvalue -= 1;
	
	CheckRange();
	
	// send message to change time
	DispatchMessage();
}
		
		
void
TTimeEdit::BuildDispatch(BMessage *message)
{
	static const char *fields[4] = {"hour", "minute", "second", "isam"};
	
	message->AddBool("time", true);
	
	for (int32 idx = 0; idx < f_sections->CountItems() -1; idx++) {
		uint32 data;
		
		if (f_focus == idx)
			data = f_holdvalue;
		else
			data = ((TDateTimeSection *)f_sections->ItemAt(idx))->Data();
		
		if (idx == 3) // isam
			message->AddBool("isam", data == 1);
		else
			message->AddInt32(fields[idx], data);
	}
}


void
TTimeEdit::CheckRange()
{
	int32 value = f_holdvalue;
	switch (f_focus) {
		case 0: //hour
			if (value> 23) 
				value = 0;
			 else if (value < 0) 
				value = 23;
		break;
		
		case 1: //minute
			if (value> 59)
				value = 0;
			else if (value < 0)
				value = 59;
		break;
		
		case 2: //second
			if (value > 59)
				value = 0;
			else if (value < 0)
				value = 59;
			
		break;
		
		case 3:
			// modify hour value to reflect change in am/pm
			value = ((TDateTimeSection *)f_sections->ItemAt(0))->Data();
			if (value < 13)
				value += 12;
			else
				value -= 12;
			if (value == 24) value = 0;
			((TDateTimeSection *)f_sections->ItemAt(0))->SetData(value);
		break;
		
		default:
		break;
	}
	
	((TDateTimeSection *)f_sections->ItemAt(f_focus))->SetData(value);
	f_holdvalue = value;
	
	Draw(Bounds());
}



/* DATE TSECTIONEDIT */


const char *months[] = 
{ "January", "Febuary", "March", "April", "May", "June",
  "July", "August", "September", "October", "November", "December" 
};
  
  
TDateEdit::TDateEdit(BRect frame, const char *name, uint32 sections)
	: TSectionEdit(frame, name, sections)
{
	InitView();
}


TDateEdit::~TDateEdit()
{
	TSection *section;
	if (f_sections->CountItems() > 0)
		for (int32 idx = 0; (section = (TSection *)f_sections->ItemAt(idx)); idx++)
			delete section;
	delete f_sections;
}


void
TDateEdit::InitView()
{
	TSectionEdit::InitView();
	SetSections(f_sectionsarea);
}


void
TDateEdit::DrawSection(uint32 index, bool isfocus)
{
	if (f_sections->CountItems() == 0)
		printf("No Sections!!!\n");

	// user defined section drawing
	TDateTimeSection *section;
	section = (TDateTimeSection *)f_sections->ItemAt(index);
	
	BRect bounds = section->Frame();
	
	
	// format value to display

	uint32 value;
	
	if (isfocus) {
		SetLowColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
		value = f_holdvalue;
	} else {
		SetLowColor(ViewColor());
		value = section->Data();
	}
	
	BString text;
	// format value (new method?)
	switch (index) {
		case 0: // month
			text << months[value];
		break;
		
		case 1: // day
			text << value;
		break;
		
		case 2: // year
			text << value +1900;
		break;
		
		default:
			return;
		break;
	}
	// calc and center text in section rect
		
	float width = be_plain_font->StringWidth(text.String());
	
	BPoint offset(-(bounds.Width()/2.0 -width/2.0) -1.0, (bounds.Height()/2.0 -6.0));
	
	if (index == 0)
		offset.x = -(bounds.Width() -width) ;
	
	BPoint drawpt(bounds.LeftBottom() -offset);
	
	SetHighColor(0, 0, 0, 255);
	FillRect(bounds, B_SOLID_LOW);
	DrawString(text.String(), drawpt);	
}


void
TDateEdit::DrawSeperator(uint32 index)
{
	/* user drawn seperator.  section is the index of the section 
	in f_sections or the section to the seps left
	*/
	
	if (index == 3) return;  // no seperator for am/pm
	BString text("/");
	uint32 sep;
	GetSeperatorWidth(&sep);
	
	TDateTimeSection *section = (TDateTimeSection *)f_sections->ItemAt(index);
	BRect bounds = section->Frame();
	
	float width = be_plain_font->StringWidth(text.String());
	BPoint offset(-(sep/2.0 -width/2.0) -1.0, bounds.Height()/2.0 -6.0);
	BPoint drawpt(bounds.RightBottom() -offset);

	DrawString(text.String(), drawpt);	
}


void
TDateEdit::SetSections(BRect area)
{
	TDateTimeSection *section;
	// create sections
	for (uint32 idx = 0; idx < f_sectioncount; idx++) {
		section = new TDateTimeSection(area);
		f_sections->AddItem(section);
	}
	
	BRect bounds(area);
	float width;
	
	// year
	width = be_plain_font->StringWidth("0000") +6;
	bounds.right = area.right;
	bounds.left = bounds.right -width;
	((TDateTimeSection *)f_sections->ItemAt(2))->SetBounds(bounds);
	
	uint32 sep;
	GetSeperatorWidth(&sep);
	
	// day
	width = be_plain_font->StringWidth("00") +6;
	bounds.right = bounds.left -sep;
	bounds.left = bounds.right -width;
	((TDateTimeSection *)f_sections->ItemAt(1))->SetBounds(bounds);
	
	// month
	bounds.right = bounds.left -sep;
	bounds.left = area.left;
	((TDateTimeSection *)f_sections->ItemAt(0))->SetBounds(bounds);
}


void
TDateEdit::GetSeperatorWidth(uint32 *width)
{
	*width = 8;
}


void
TDateEdit::SectionFocus(uint32 index)
{
	f_focus = index;

	// update hold value
	f_holdvalue = ((TDateTimeSection *)f_sections->ItemAt(f_focus))->Data();
	
	Draw(Bounds());
}


void
TDateEdit::SetTo(uint32 year, uint32 month, uint32 day)
{
	if (f_sections->CountItems()> 0)
	{
		bool update = false;
		
		TDateTimeSection *section = (TDateTimeSection *)f_sections->ItemAt(0);
		if (section->Data() != month) {
			section->SetData(month);
			update = true;
		}
		
		section = (TDateTimeSection *)f_sections->ItemAt(1);
		if (section->Data() != day) {
			section->SetData(day);
			update = true;
		}
			
		section = (TDateTimeSection *)f_sections->ItemAt(2);
		if (section->Data() != year) {
			section->SetData(year);
			update = true;
		}
		
		if (update)
			Draw(Bounds());
	}
}


void
TDateEdit::DoUpPress()
{
	// update displayed value
	f_holdvalue += 1;
	
	CheckRange();
	
	// send message to change Date
	DispatchMessage();
}

		
void
TDateEdit::DoDownPress()
{
	// update display value
	f_holdvalue -= 1;
	
	CheckRange();
	
	// send message to change Date
	DispatchMessage();
}
		
		
void
TDateEdit::BuildDispatch(BMessage *message)
{
	static const char *fields[3] = {"month", "day", "year"};
	
	message->AddBool("time", false);
	
	for (int32 idx = 0; idx < f_sections->CountItems(); idx++) {
		uint32 data;
		
		if (f_focus == idx)
			data = f_holdvalue;
		else
			data = ((TDateTimeSection *)f_sections->ItemAt(idx))->Data();
		
		message->AddInt32(fields[idx], data);
	}
}


void
TDateEdit::CheckRange()
{
	int32 value = f_holdvalue;
	
	switch (f_focus) {
		case 0: // month
		{
			if (value > 11)
				value = 0;
			else if (value < 0)
				value = 11;
		}
		break;
		
		case 1: //day
		{
			uint32 month = ((TDateTimeSection *)f_sections->ItemAt(0))->Data();
			uint32 year = ((TDateTimeSection *)f_sections->ItemAt(2))->Data();
			
			int daycnt = getDaysInMonth(month, year);
			if (value > daycnt)
				value = 1;
			else if (value < 1)
				value = daycnt;
		}
		break;
		
		case 2: //year
		{
			if (value > YEAR_DELTA_MAX)
				value = YEAR_DELTA_MIN;
			else if (value < YEAR_DELTA_MIN)
				value = YEAR_DELTA_MAX;
		}
		break;
		
		default:
		break;
	}
	
	((TDateTimeSection *)f_sections->ItemAt(f_focus))->SetData(value);
	f_holdvalue = value;
	
	Draw(Bounds());
}


//---//
