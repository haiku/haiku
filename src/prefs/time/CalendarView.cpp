#include <Debug.h>
#include <Message.h>
#include <String.h>

#include <stdio.h>

#include "CalendarView.h"

#define SEGMENT_CHANGE 'ostd'

void

__printrect(BRect rect)
{
	printf("%f, %f, %f, %f\n", rect.left, rect.top, rect.right, rect.bottom);
}

TDay::TDay(BRect frame, int day) 
	: BView(frame, B_EMPTY_STRING, B_FOLLOW_NONE, B_WILL_DRAW)
	, f_day(day)
	, f_isselected(false)
{	}

TDay::~TDay()
{	}

void
TDay::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());
}

void
TDay::MouseDown(BPoint where)
{
	if (f_day> 0)
	{
		f_isselected = !f_isselected;
		Draw(Bounds());	
	}
}

void
TDay::Stroke3dFrame(BRect frame, rgb_color light, rgb_color dark, bool inset)
{
	rgb_color color1;
	rgb_color color2;
	
	if (inset)
	{
		color1 = dark;
		color2 = light;
	}
	else
	{
		color1 = light;
		color2 = dark;
	}
	
	BeginLineArray(4);
		AddLine(frame.LeftTop(), frame.RightTop(), color1);
		AddLine(frame.LeftTop(), frame.LeftBottom(), color1);
		AddLine(frame.LeftBottom(), frame.RightBottom(), color2);
		AddLine(frame.RightBottom(), frame.RightTop(), color2);
	EndLineArray();
}


void
TDay::Draw(BRect updaterect)
{
	BRect bounds(Bounds());
	
	SetHighColor(ViewColor());
	FillRect(Bounds(), B_SOLID_HIGH);
	
	rgb_color bgcolor;
	rgb_color dark;
	rgb_color light;
	
	BString text;
	
	if (f_isselected)
	{
		bgcolor = tint_color(ViewColor(), B_DARKEN_2_TINT);		
	}
	else
	{
		bgcolor = tint_color(ViewColor(), B_DARKEN_1_TINT);
	}
	text << f_day;

	dark = tint_color(bgcolor, B_DARKEN_1_TINT);
	light = tint_color(bgcolor, B_LIGHTEN_MAX_TINT);
	
	SetHighColor(bgcolor);
	FillRect(bounds);

	if (f_isselected)
		Stroke3dFrame(bounds, light, dark, true);
	
	if (f_day> 0)
	{
		if (!f_isselected)
			SetHighColor(0, 0, 0, 255);
		else
			SetHighColor(255, 255, 255, 255);
			
		SetLowColor(bgcolor);
		
		// calc font size;
		float width = be_plain_font->StringWidth(text.String());
		font_height finfo;
		be_plain_font->GetHeight(&finfo);
		float height = finfo.descent +finfo.ascent +finfo.leading;

		BPoint drawpt((bounds.Width()/2.0) -(width/2.0) +1, (bounds.Height()/2.0) +(height/2.0) -2);
		DrawString(text.String(), drawpt);
	}
}

void 
TDay::SetTo(BRect frame, int day)
{
	MoveTo(frame.LeftTop());
	f_day = day;
	Draw(Bounds());
}

void
TDay::SetDay(int day, bool selected = false)
{
	f_day = day;
	f_isselected = selected;
	Draw(Bounds());
}


/*=====> TCalendarView <=====*/

TCalendarView::TCalendarView(BRect frame, const char *name, uint32 resizingmode, uint32 flags)
	: BView(frame, name, resizingmode, flags)
{
	InitView();
}

TCalendarView::~TCalendarView()
{
}

void
TCalendarView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());
}


const char days[7] = { 'S', 'M', 'T', 'W', 'T', 'F', 'S' };

void
TCalendarView::Draw(BRect updaterect)
{	
	BRect bounds(0.0, 0.0, f_dayrect.Width(), f_dayrect.Height());

	float width = 0;	
	float x = bounds.Width()/2.0;
	BPoint drawpt;
	BString day;
	SetLowColor(ViewColor());
	SetHighColor(0, 0, 0);
	for (int i = 0; i < 7; i++)
	{
		day = days[i];
		width = be_plain_font->StringWidth(day.String());
		drawpt.x = bounds.left +(x -width/2.0 +2);
		drawpt.y = bounds.bottom;
		DrawString(day.String(), drawpt);	
		bounds.OffsetBy(bounds.Width() +1, 0);
	}
}

void
TCalendarView::ClearDays()
{
	BView *child; 
	if (CountChildren()> 0)
	{
		if ((child = ChildAt(0)) != NULL)
		{
				while (child)
			{
				RemoveChild(child);
				child = child->NextSibling();
			}
		}
	}
}

void
TCalendarView::CalcFlags()
{
}

static bool
isLeapYear(const int year)
{
	if ((year % 400 == 0)||(year % 4 == 0 && year % 100 == 0))
		return true;
	else
		return false;
}

static int
GetDaysInMonth(const int month, const int year)
{
	if (month == 2 && isLeapYear(year))
	{
		return 29;
	}
	
	static const int DaysinMonth[12] = 
		{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	
	return DaysinMonth[month];
}

static int
GetFirstDay(const int month, const int year)
{
	int l_century = year/100;
	int l_decade = year%100;
	int l_month = (month +10)%12;
	int l_day = 1;
	
	if (l_month == 1 || l_month == 2)
	{
		if (l_decade == 0)
		{
			l_decade = 99;
			l_century -= 1;
		}
		else
		 l_decade-= 1;
	}
	
	float tmp = (l_day +(floor(((13 *l_month) -1)/5)) 
		+l_decade +(floor(l_decade /4)) 
		+(floor(l_century/4)) -(2 *l_century));
	int result = static_cast<int>(tmp)%7;
	
	if (result < 0)
		result += 7;
		
	return result;
}

void
TCalendarView::InitView()
{
	font_height finfo;
	be_plain_font->GetHeight(&finfo);
	float text_height = finfo.ascent +finfo.descent +finfo.leading;
	
	BRect bounds(Bounds());
	float width = (bounds.Width() -7)/7;
	float height = ((bounds.Height() -(text_height +4)) -6)/6;
	f_dayrect.Set(0.0, text_height +6, width, text_height +6 +height);

	__printrect(f_dayrect);

	BRect dayrect(f_dayrect);
	TDay *day;
	for (int row = 0; row < 6; row++)
	{
		for (int col = 0; col < 7; col++)
		{
			day = new TDay(dayrect.InsetByCopy(1, 1), 0);
			AddChild(day);
			dayrect.OffsetBy(width +1, 0);
		}
		dayrect.OffsetBy(0, height +1);
		dayrect.OffsetTo(0, dayrect.top);
	}
}

void
TCalendarView::InitDates()
{
	TDay *day;
	
	int label = 0;
	int idx = 0;
	int first = GetFirstDay(f_month+1, f_year);
	int daycnt = GetDaysInMonth(f_month, f_year);
	printf("%d, %d\n", first, daycnt);
	for (int row = 0; row < 6; row++)
	{
		for (int col = 0; col < 7; col++)
		{
			day = dynamic_cast<TDay *>(ChildAt((row *7) +col));
			
			if (idx < first || idx > daycnt +first +1)
				label = 0;
			else
				label = idx -(first +1);
			idx++;
			day->SetDay(label, (label == f_day));
			
		}
	}
}

void
TCalendarView::Update(tm *atm)
{
	if (f_month != atm->tm_mon
	 ||f_day != atm->tm_mday
	 ||f_year != atm->tm_year)
	{
		f_month = atm->tm_mon;
		f_day = atm->tm_mday;
		f_wday = atm->tm_wday;
		f_year = atm->tm_year;
		InitDates();
	}
	printf("M: %d %d D: %d %d Y: %d %d\n", 
	f_month, atm->tm_mon, f_day, atm->tm_mday, f_year, atm->tm_year);
}
