#include <Bitmap.h>
#include <String.h>

#include "DateTimeEdit.h"
#include "Bitmaps.h"

const int32 kArrowAreaWidth = 16;

/*=====> TDateTimeEdit <=====*/
TDateTimeEdit::TDateTimeEdit(BRect frame, const char *name)
	: BView(frame, name, B_FOLLOW_NONE, B_NAVIGABLE|B_WILL_DRAW)
{	
	f_up = new BBitmap(BRect(0, 0, kUpArrowWidth -1, kUpArrowHeight -1), kUpArrowColorSpace);
	f_up->SetBits(kUpArrowBits, (kUpArrowWidth) *(kUpArrowHeight+1), 0, kUpArrowColorSpace);
	ReplaceTransparentColor(f_up, ui_color(B_PANEL_BACKGROUND_COLOR)); // should be set to parents back color
	
	f_down = new BBitmap(BRect(0, 0, kDownArrowWidth -1, kDownArrowHeight -2), kDownArrowColorSpace);
	f_down->SetBits(kDownArrowBits, (kDownArrowWidth) *(kDownArrowHeight), 0, kDownArrowColorSpace);
	ReplaceTransparentColor(f_down, ui_color(B_PANEL_BACKGROUND_COLOR));
	
}

TDateTimeEdit::~TDateTimeEdit()
{	}

void
TDateTimeEdit::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());
}

void
TDateTimeEdit::MouseDown(BPoint where)
{
	MakeFocus(true);
	if (f_uprect.Contains(where))
		UpPress();
	else
	if (f_downrect.Contains(where))
		DownPress();
}

void
TDateTimeEdit::UpPress()
{	}

void
TDateTimeEdit::DownPress()
{	}

void
TDateTimeEdit::MakeFocus(bool focused)
{
	if (focused != IsFocus())
	{
		BView::MakeFocus(focused);
		Draw(Bounds());
		Flush();
	}
}

void
TDateTimeEdit::Draw(BRect updaterect)
{
	rgb_color bgcolor = ViewColor();
	rgb_color light = tint_color(bgcolor, B_LIGHTEN_MAX_TINT);
	rgb_color dark = tint_color(bgcolor, B_DARKEN_1_TINT);
	rgb_color darker = tint_color(bgcolor, B_DARKEN_3_TINT);

	// draw 3d border
	BRect bounds(Bounds());
	SetHighColor(light);
	SetLowColor(dark);
	Draw3dFrame(bounds, true);
	StrokeLine(bounds.LeftBottom(), bounds.LeftBottom(), B_SOLID_LOW);
	
	bounds.InsetBy(1, 1);
	bounds.right -= kArrowAreaWidth;
	if (IsFocus())
	{
		rgb_color navcolor = keyboard_navigation_color();
	
		SetHighColor(navcolor);
		StrokeRect(bounds);
	}
	else
	{
		// draw border thickening (erase focus)
		SetHighColor(darker);
		SetLowColor(bgcolor);	
		Draw3dFrame(bounds, false);
	}

	// draw up/down control
	SetHighColor(light);
	bounds.left = bounds.right +1;// -kArrowAreaWidth;
	bounds.right = Bounds().right -1;
	f_uprect.Set(bounds.left +3, bounds.top +2, bounds.right, bounds.bottom /2.0);
	f_downrect = f_uprect.OffsetByCopy(0, f_uprect.Height()+2);
	
	DrawBitmap(f_up, f_uprect.LeftTop());
	DrawBitmap(f_down, f_downrect.LeftTop());
	
	Draw3dFrame(bounds, false);
	SetHighColor(dark);
	StrokeLine(bounds.LeftBottom(), bounds.RightBottom());
	StrokeLine(bounds.RightBottom(), bounds.RightTop());
	SetHighColor(light);
	StrokeLine(bounds.RightTop(), bounds.RightTop());
}

void
TDateTimeEdit::Draw3dFrame(BRect frame, bool inset)
{
	rgb_color color1, color2;
	
	if (inset)
	{
		color1 = HighColor();
		color2 = LowColor();
	}
	else
	{
		color1 = LowColor();
		color2 = HighColor();
	}
	
	BeginLineArray(6);
		// left side
		AddLine(frame.LeftBottom(), frame.LeftTop(), color2);
		// right side
		AddLine(frame.RightTop(), frame.RightBottom(), color1);
		// bottom side
		AddLine(frame.RightBottom(), frame.LeftBottom(), color1);
		// top side
		AddLine(frame.LeftTop(), frame.RightTop(), color2);
	EndLineArray();
}


/*=====> TTimeEdit <=====*/

TTimeEdit::TTimeEdit(BRect frame, const char *name)
	: TDateTimeEdit(frame, name)
{
	BRect bounds(Bounds().InsetByCopy(1, 2));

	// all areas same width :)
	float width = be_plain_font->StringWidth("00") +7;

	// AM/PM rect
	bounds.right = Bounds().right -(kArrowAreaWidth +3);
	bounds.left = bounds.right -(width +4);
	f_aprect = bounds;	
	

	// seconds rect
	bounds.right = bounds.left;
	bounds.left = bounds.right -width;
	f_secrect = bounds;
	
	float sep_width = 9; 

	//minutes rect
	bounds.right = bounds.left -sep_width;
	bounds.left = bounds.right -width;	
	f_minrect = bounds;
	
	//hour rect
	bounds.right = bounds.left -sep_width;
	bounds.left = Bounds().left +2;
	f_hourrect = bounds;
}

TTimeEdit::~TTimeEdit()
{	}

void
TTimeEdit::Draw(BRect updaterect)
{
	TDateTimeEdit::Draw(Bounds());
	
	SetHighColor(0, 0, 0, 255);
	
	BRect bounds(Bounds());
	BString text;
	BPoint drawpt;	
	BPoint offset(0, f_hourrect.Height()/2.0 -6);

	// seperator
	BString septext(":");
	
	float width;
	rgb_color dark = tint_color(ViewColor(), B_DARKEN_1_TINT);
	
	//draw hour
	if (updaterect.Contains(f_hourrect))
	{
		if (f_hour> 12)
		{
			if (f_hour < 22) text << "0";
			text << f_hour -12;
		}
		else
		{
			if (f_hour < 10) text << "0";
			text << f_hour;
		}
		width = be_plain_font->StringWidth(text.String());
		offset.x = -(f_hourrect.Width()/2.0 -width/2.0);
		drawpt = f_hourrect.LeftBottom() -offset;
	
		if (f_focus == FOCUS_HOUR)
			SetLowColor(dark);
		else
			SetLowColor(ViewColor());

		FillRect(f_hourrect, B_SOLID_LOW);
		DrawString(text.String(), drawpt);
		
		offset.x = -3;
		SetLowColor(ViewColor());
		DrawString(septext.String(), f_hourrect.RightBottom() -offset);
	}
			
	//draw min
	if (updaterect.Contains(f_minrect))
	{
		text = "";
		if (f_minute < 10) text << "0";
		text << f_minute;
		width = be_plain_font->StringWidth(text.String());
		offset.x = -(f_minrect.Width()/2.0 -width/2.0);
		drawpt = f_minrect.LeftBottom() -offset;

		if (f_focus == FOCUS_MINUTE)
			SetLowColor(dark);
		else
			SetLowColor(ViewColor());
		
		FillRect(f_minrect, B_SOLID_LOW);
		DrawString(text.String(), drawpt);
		
		offset.x = -3;
		SetLowColor(ViewColor());
		DrawString(septext.String(), f_minrect.RightBottom() -offset);
	}
			
	//draw sec
	if (updaterect.Contains(f_secrect))
	{
		text = "";
		if (f_second < 10) text << "0";
		text << f_second;
		width = be_plain_font->StringWidth(text.String());
		offset.x = -(f_secrect.Width()/2.0 -width/2.0);
		drawpt = f_secrect.LeftBottom() -offset;

		if (f_focus == FOCUS_SECOND)
			SetLowColor(dark);
		else
			SetLowColor(ViewColor());

		FillRect(f_secrect, B_SOLID_LOW);
		DrawString(text.String(), drawpt);
	}
			
	//draw AM/PM
	if (updaterect.Contains(f_aprect))
	{
		text = "";
		if (f_isam)
			text << "AM";
		else
			text << "PM";
			
		width = be_plain_font->StringWidth(text.String());
		offset.x = -(f_aprect.Width()/2.0 -width/2.0);
		drawpt = f_aprect.LeftBottom() -offset;
		
		if (f_focus == FOCUS_AMPM)
			SetLowColor(dark);
		else
			SetLowColor(ViewColor());
	
		FillRect(f_aprect, B_SOLID_LOW);
		DrawString(text.String(), drawpt);
	}
}

void
TTimeEdit::MouseDown(BPoint where)
{
	TDateTimeEdit::MouseDown(where);
	
	if (f_hourrect.Contains(where))
	{
		f_focus = FOCUS_HOUR;
	}
	else
	if (f_minrect.Contains(where))
	{
		f_focus = FOCUS_MINUTE;
	}
	else
	if (f_secrect.Contains(where))
	{
		f_focus = FOCUS_SECOND;
	}
	else
	if (f_aprect.Contains(where))
	{
		f_focus = FOCUS_AMPM;
	}
	else
		f_focus = FOCUS_NONE;
		
	Draw(Bounds());
}

void
TTimeEdit::UpPress()
{
	switch(f_focus)
	{
		case FOCUS_HOUR:
			f_hour += 1;
			Draw(f_hourrect);
			break;
		case FOCUS_MINUTE:
			f_minute += 1;
			Draw(f_minrect);
			break;
		case FOCUS_SECOND:
			f_second += 1;
			Draw(f_secrect);
			break;
		case FOCUS_AMPM:
			f_isam = !f_isam;
			Draw(f_aprect);
			break;
		default:
			break;
	}
}

void
TTimeEdit::DownPress()
{
	switch(f_focus)
	{
		case FOCUS_HOUR:
			f_hour -= 1;
			Draw(f_hourrect);
			break;
		case FOCUS_MINUTE:
			f_minute -= 1;
			Draw(f_minrect);
			break;
		case FOCUS_SECOND:
			f_second -= 1;
			Draw(f_secrect);
			break;
		case FOCUS_AMPM:
			f_isam = !f_isam;
			Draw(f_aprect);
			break;
		default:
			break;
	}
}


void
TTimeEdit::SetTo(int hour, int minute, int second)
{
	f_hour = hour;
	f_minute = minute;
	f_second = second;
	f_isam = f_hour <= 12;
	Draw(Bounds());
}

void
TTimeEdit::Update(tm *_tm)
{
	if (f_hour != _tm->tm_hour
	 ||f_minute != _tm->tm_min
	 ||f_second != _tm->tm_sec)
	{
		SetTo(_tm->tm_hour, _tm->tm_min, _tm->tm_sec);
	}
}


/*=====> TDateEdit <=====*/

const char *months[] = 
{ "January", "Febuary", "March", "April", "May", "June",
  "July", "August", "September", "October", "November", "December" };
  
TDateEdit::TDateEdit(BRect frame, const char *name)
	: TDateTimeEdit(frame, name)
{	
	BRect bounds(Bounds().InsetByCopy(1, 2));
	float width;

	// year rect
	width = be_plain_font->StringWidth("0000") +8;
	bounds.right = Bounds().right -(kArrowAreaWidth +2);
	bounds.left = bounds.right -width;
	f_yearrect = bounds;	
	
	// don't know how to determine system date sep char
	float sep_width = be_plain_font->StringWidth("/") +4; 
	
	// day rect
		//day is 2 digits 0 usually widest
	width = be_plain_font->StringWidth("00") +4;
	bounds.right = bounds.left -sep_width;
	bounds.left = bounds.right -width;
	f_dayrect = bounds;
	
	//get september width (longest)
	bounds.right = bounds.left -sep_width;
	bounds.left = Bounds().left +2;
	
	f_monthrect = bounds;


}

TDateEdit::~TDateEdit()
{	}

void
TDateEdit::MouseDown(BPoint where)
{
	TDateTimeEdit::MouseDown(where);
	
	if (f_monthrect.Contains(where))
	{
		f_focus = FOCUS_MONTH;
	}
	else
	if (f_dayrect.Contains(where))
	{
		f_focus = FOCUS_DAY;
	}
	else
	if (f_yearrect.Contains(where))
	{
		f_focus = FOCUS_YEAR;
	}
	else
		f_focus = FOCUS_NONE;
	
	Draw(Bounds());
}

void
TDateEdit::Draw(BRect updaterect)
{
	TDateTimeEdit::Draw(Bounds());
	
	SetHighColor(0, 0, 0, 255);
	
	BRect bounds(Bounds());
	BString text;
	BPoint drawpt;	
	BPoint offset(0, f_monthrect.Height()/2.0 -6);

	// seperator
	BString septext("/");
	
	float width;
	rgb_color dark = tint_color(ViewColor(), B_DARKEN_1_TINT);
	
	//draw month
	if (updaterect.Contains(f_monthrect))
	{
		text << months[f_month];
		width = be_plain_font->StringWidth(text.String());
		offset.x = width +4;
		drawpt = f_monthrect.RightBottom() -offset;
	
		if (f_focus == FOCUS_MONTH)
			SetLowColor(dark);
		else
			SetLowColor(ViewColor());

		FillRect(f_monthrect, B_SOLID_LOW);
		DrawString(text.String(), drawpt);
		
		offset.x = -2;
		SetLowColor(ViewColor());
		DrawString(septext.String(), f_monthrect.RightBottom() -offset);
	}
	
	//draw day
	if (updaterect.Contains(f_dayrect))
	{
		text = "";
		text << f_day;
		width = be_plain_font->StringWidth(text.String());
		offset.x = -(f_dayrect.Width()/2.0 -width/2.0);
	
		drawpt = f_dayrect.LeftBottom() -offset;
		if (f_focus == FOCUS_DAY)
			SetLowColor(dark);
		else
			SetLowColor(ViewColor());
		
		FillRect(f_dayrect, B_SOLID_LOW);
		DrawString(text.String(), drawpt);
		
		offset.x = -2;
		SetLowColor(ViewColor());
		DrawString(septext.String(), f_dayrect.RightBottom() -offset);
	}
			
	//draw year
	if (updaterect.Contains(f_yearrect))
	{
		text = "";
		text << f_year +1900;
		width = be_plain_font->StringWidth(text.String());
		offset.x = -(f_yearrect.Width()/2.0 -width/2.0);
		drawpt = f_yearrect.LeftBottom() -offset;
		
		if (f_focus == FOCUS_YEAR)
			SetLowColor(dark);
		else
			SetLowColor(ViewColor());
	
		FillRect(f_yearrect, B_SOLID_LOW);
		DrawString(text.String(), drawpt);
	}
}

void
TDateEdit::UpPress()
{
	switch(f_focus)
	{
		case FOCUS_MONTH:
			f_month += 1;
			Draw(f_monthrect);
			break;
		case FOCUS_DAY:
			f_day += 1;
			Draw(f_dayrect);
			break;
		case FOCUS_YEAR:
			f_year += 1;
			Draw(f_yearrect);
			break;
		default:
			break;
	}
}

void
TDateEdit::DownPress()
{
	switch(f_focus)
	{
		case FOCUS_MONTH:
			f_month -= 1;
			Draw(f_monthrect);
			break;
		case FOCUS_DAY:
			f_day -= 1;
			Draw(f_dayrect);
			break;
		case FOCUS_YEAR:
			f_year -= 1;
			Draw(f_yearrect);
			break;
		default:
			break;
	}
}

void
TDateEdit::SetTo(int month, int day, int year)
{
	f_month = month;
	f_day = day;
	f_year = year;
	Draw(Bounds());
}

void
TDateEdit::Update(tm *_tm)
{
	if (f_month != _tm->tm_mon
	 || f_day != _tm->tm_mday
	 || f_year != _tm->tm_year)
	{
		f_month = _tm->tm_mon;
		f_day = _tm->tm_mday;
		f_year = _tm->tm_year;
		Draw(Bounds());
	}
}
