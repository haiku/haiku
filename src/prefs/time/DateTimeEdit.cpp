#include <Bitmap.h>
#include <String.h>
#include <Window.h>

#include "Bitmaps.h"
#include "DateTimeEdit.h"
#include "DateUtils.h"
#include "TimeMessages.h"

const int32 kArrowAreaWidth = 16;

// number of years after 1900 to loop should be system setting
#define YEAR_DELTA 110

/*=====> TDateTimeEdit <=====*/

TDateTimeEdit::TDateTimeEdit(BRect frame, const char *name)
	: BControl(frame, name, NULL, NULL, B_FOLLOW_NONE, B_NAVIGABLE|B_WILL_DRAW)
	, f_holdvalue(0)
{	
	f_up = new BBitmap(BRect(0, 0, kUpArrowWidth -1, kUpArrowHeight -1), kUpArrowColorSpace);
	f_up->SetBits(kUpArrowBits, (kUpArrowWidth) *(kUpArrowHeight+1), 0, kUpArrowColorSpace);
	
	f_down = new BBitmap(BRect(0, 0, kDownArrowWidth -1, kDownArrowHeight -2), kDownArrowColorSpace);
	f_down->SetBits(kDownArrowBits, (kDownArrowWidth) *(kDownArrowHeight), 0, kDownArrowColorSpace);
	
}


TDateTimeEdit::~TDateTimeEdit()
{
}


void
TDateTimeEdit::AttachedToWindow()
{
	if (Parent()) {
		SetViewColor(Parent()->ViewColor());
		ReplaceTransparentColor(f_up, ViewColor());
		ReplaceTransparentColor(f_down, ViewColor());
	}
}


void
TDateTimeEdit::MessageReceived(BMessage *message)
{
	switch(message->what) {
		default:
			BView::MessageReceived(message);
			break;
	}
}


void
TDateTimeEdit::MouseDown(BPoint where)
{
	MakeFocus(true);
	if (f_uprect.Contains(where))
		UpPress();
	else if (f_downrect.Contains(where))
		DownPress();
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

	if ( (IsFocus() && Window() && Window()->IsActive())) {
		rgb_color navcolor = keyboard_navigation_color();
	
		SetHighColor(navcolor);
		StrokeRect(bounds);
	} else {
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
	
	if (inset) {
		color1 = HighColor();
		color2 = LowColor();
	} else {
		color1 = LowColor();
		color2 = HighColor();
	}
	
	BeginLineArray(4);
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


void
TDateTimeEdit::DispatchMessage()
{
	// send message to update timedate
	BMessage *msg = new BMessage(OB_USER_CHANGE);
	BuildDispatch(msg);
	Window()->PostMessage(msg);
}


/*=====> TTimeEdit <=====*/

TTimeEdit::TTimeEdit(BRect frame, const char *name)
	: TDateTimeEdit(frame, name)
{
	BRect bounds(Bounds().InsetByCopy(1, 2));

	// all areas same width :)
	float width = be_plain_font->StringWidth("00") +6;

	// AM/PM rect
	bounds.right = Bounds().right -(kArrowAreaWidth +3);
	bounds.left = bounds.right -(width +5);
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
{
}


void
TTimeEdit::Draw(BRect updaterect)
{
	TDateTimeEdit::Draw(Bounds());
	
	SetHighColor(0, 0, 0, 255);
	
	BString text;
	BPoint drawpt;	
	BPoint offset(0, f_hourrect.Height()/2.0 -6);
	int value;
	
	// seperator
	BString septext(":");
	
	float width;
	rgb_color dark = tint_color(ViewColor(), B_DARKEN_1_TINT);
	
	//draw hour
	if (updaterect.Contains(f_hourrect)) {
		
		if (f_focus == FOCUS_HOUR && IsFocus()) {
			SetLowColor(dark);
			value = f_holdvalue;
		} else {
			SetLowColor(ViewColor());
			value = f_hour;
		}
		
		if (value> 12) {
			if (value < 22) text << "0";
			text << value -12;
		} else {
			if (value < 10) text << "0";
			text << value;
		}
		width = be_plain_font->StringWidth(text.String());
		offset.x = -(f_hourrect.Width()/2.0 -width/2.0) -1;
		drawpt = f_hourrect.LeftBottom() -offset;
	
		FillRect(f_hourrect, B_SOLID_LOW);
		DrawString(text.String(), drawpt);
		
		SetLowColor(ViewColor());
		DrawString(septext.String(), f_hourrect.RightBottom() -offset);
	}
			
	//draw min
	if (updaterect.Contains(f_minrect)) {	
		if (f_focus == FOCUS_MINUTE && IsFocus()) {
			SetLowColor(dark);
			value = f_holdvalue;
		} else {
			SetLowColor(ViewColor());
			value = f_minute;
		}
		
		text = "";
		if (value < 10) text << "0";
		text << value;
		
		width = be_plain_font->StringWidth(text.String());
		offset.x = -(f_minrect.Width()/2.0 -width/2.0) -1;
		drawpt = f_minrect.LeftBottom() -offset;

		FillRect(f_minrect, B_SOLID_LOW);
		DrawString(text.String(), drawpt);
		
		offset.x = -3;
		SetLowColor(ViewColor());
		DrawString(septext.String(), f_minrect.RightBottom() -offset);
	}
			
	//draw sec
	if (updaterect.Contains(f_secrect)) {
		if (f_focus == FOCUS_SECOND && IsFocus()) {
			SetLowColor(dark);
			value = f_holdvalue;
		} else {
			SetLowColor(ViewColor());
			value = f_second;
		}
			
		text = "";
		if (value < 10) text << "0";
		text << value;
		
		width = be_plain_font->StringWidth(text.String());
		offset.x = -(f_secrect.Width()/2.0 -width/2.0) -1;
		drawpt = f_secrect.LeftBottom() -offset;

		FillRect(f_secrect, B_SOLID_LOW);
		DrawString(text.String(), drawpt);
	}
			
	//draw AM/PM
	if (updaterect.Contains(f_aprect)) {
		text = "";
		if (f_isam)
			text << "AM";
		else
			text << "PM";
			
		width = be_plain_font->StringWidth(text.String());
		offset.x = -(f_aprect.Width()/2.0 -width/2.0) -1;
		drawpt = f_aprect.LeftBottom() -offset;
		
		if (f_focus == FOCUS_AMPM && IsFocus())
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
	
	if (f_hourrect.Contains(where)) {
		f_focus = FOCUS_HOUR;
		f_holdvalue = f_hour;
	} else if (f_minrect.Contains(where)) {
		f_focus = FOCUS_MINUTE;
		f_holdvalue = f_minute;
	} else if (f_secrect.Contains(where)) {
		f_focus = FOCUS_SECOND;
		f_holdvalue = f_second;
	} else if (f_aprect.Contains(where)) {
		f_focus = FOCUS_AMPM;
	}		
	Draw(Bounds());
}


void
TTimeEdit::SetFocus(bool forward)
{
	switch(f_focus) {
		case FOCUS_HOUR:
			(forward)?f_focus = FOCUS_MINUTE: f_focus = FOCUS_AMPM;
			break;
		case FOCUS_MINUTE:
			(forward)?f_focus = FOCUS_SECOND: f_focus = FOCUS_HOUR;
			break;
		case FOCUS_SECOND:
			(forward)?f_focus = FOCUS_AMPM: f_focus = FOCUS_MINUTE;
			break;
		case FOCUS_AMPM:
			(forward)?f_focus = FOCUS_HOUR: f_focus = FOCUS_SECOND;
			break;
		default:
			break;
	}
	
	UpdateFocus();
}


void
TTimeEdit::UpdateFocus()
{
	switch(f_focus) {
		case FOCUS_HOUR:
			f_hour = f_holdvalue;
			Draw(f_hourrect);
			break;
		case FOCUS_MINUTE:
			f_minute = f_holdvalue;
			Draw(f_minrect);
			break;
		case FOCUS_SECOND:
			f_second = f_holdvalue;
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
TTimeEdit::KeyDown(const char *bytes, int32 numbytes)
{
	switch(bytes[0]) {
		case B_LEFT_ARROW:
			SetFocus(false);
		break;
		
		case B_RIGHT_ARROW:
			SetFocus(true);
		break;
		
		case B_UP_ARROW:
			UpPress();
		break;
		
		case B_DOWN_ARROW:
			DownPress();
		break;
		
		default:
			BView::KeyDown(bytes, numbytes);
		break;
	}
}


void
TTimeEdit::UpPress()
{
	f_holdvalue += 1;
	if (f_focus == FOCUS_AMPM) 
		f_isam = !f_isam;

	UpdateFocus();

	// notify of change
	DispatchMessage();
}


void
TTimeEdit::DownPress()
{
	f_holdvalue -= 1;
	
	if (f_focus == FOCUS_AMPM) 
		f_isam = !f_isam;

	UpdateFocus();

	// notify of change
	DispatchMessage();
}


void
TTimeEdit::SetTo(int32 hour, int32 minute, int32 second)
{
	if (f_hour != hour
	  ||f_minute != minute
	  ||f_second != second) {
		f_hour = hour;
		f_minute = minute;
		f_second = second;
		f_isam = f_hour <= 12;
		Draw(Bounds());
	}
}


void
TTimeEdit::BuildDispatch(BMessage *message)
{
	message->AddBool("time", true);
	message->AddInt32("hour", f_hour);
	message->AddInt32("minute", f_minute);
	message->AddInt32("second", f_second);
}



/*=====> TDateEdit <=====*/

const char *months[] = 
{ "January", "Febuary", "March", "April", "May", "June",
  "July", "August", "September", "October", "November", "December" 
};
  
  
TDateEdit::TDateEdit(BRect frame, const char *name)
	: TDateTimeEdit(frame, name)
{	
	BRect bounds(Bounds().InsetByCopy(1, 2));
	float width;

	// year rect
	width = be_plain_font->StringWidth("0000") +6;
	bounds.right = Bounds().right -(kArrowAreaWidth +2);
	bounds.left = bounds.right -width;
	f_yearrect = bounds;	
	
	// don't know how to determine system date sep char
	float sep_width = be_plain_font->StringWidth("/") +6; 
	
	// day rect
		//day is 2 digits 0 usually widest
	width = be_plain_font->StringWidth("00") +6;
	bounds.right = bounds.left -sep_width;
	bounds.left = bounds.right -width;
	f_dayrect = bounds;
	
	bounds.right = bounds.left -sep_width;
	bounds.left = Bounds().left +2;
	
	f_monthrect = bounds;
}


TDateEdit::~TDateEdit()
{
}


void
TDateEdit::MouseDown(BPoint where)
{
	TDateTimeEdit::MouseDown(where);
	
	if (f_monthrect.Contains(where)) {
		f_focus = FOCUS_MONTH;
		f_holdvalue = f_month;
	} else if (f_dayrect.Contains(where)) {
		f_focus = FOCUS_DAY;
		f_holdvalue = f_day;
	} else if (f_yearrect.Contains(where)) {
		f_focus = FOCUS_YEAR;
		f_holdvalue = f_year;
	}
	Draw(Bounds());
}


void
TDateEdit::Draw(BRect updaterect)
{
	TDateTimeEdit::Draw(Bounds());
	
	SetHighColor(0, 0, 0, 255);
	
	BString text;
	BPoint drawpt;	
	BPoint offset(0, f_monthrect.Height()/2.0 -6);
	int value;
	
	// seperator
	BString septext("/");
	
	float width;
	rgb_color dark = tint_color(ViewColor(), B_DARKEN_1_TINT);
	
	//draw month
	if (updaterect.Contains(f_monthrect)) {
		if (f_focus == FOCUS_MONTH && IsFocus()) {
			SetLowColor(dark);
			value = f_holdvalue;
		} else {
			SetLowColor(ViewColor());
			value = f_month;
		}

		text << months[value];
		width = be_plain_font->StringWidth(text.String());
		offset.x = width +3;
		drawpt = f_monthrect.RightBottom() -offset;
	
		FillRect(f_monthrect, B_SOLID_LOW);
		DrawString(text.String(), drawpt);
		
		offset.x = -3;
		SetLowColor(ViewColor());
		DrawString(septext.String(), f_monthrect.RightBottom() -offset);
	}
	
	//draw day
	if (updaterect.Contains(f_dayrect)) {
		if (f_focus == FOCUS_DAY && IsFocus()) {
			SetLowColor(dark);
			value = f_holdvalue;
		} else {
			SetLowColor(ViewColor());
			value = f_day;
		}
		
		text = "";
		text << value;
		width = be_plain_font->StringWidth(text.String());
		offset.x = -(f_dayrect.Width()/2.0 -width/2.0) -1;
	
		drawpt = f_dayrect.LeftBottom() -offset;
		FillRect(f_dayrect, B_SOLID_LOW);
		DrawString(text.String(), drawpt);
		
		offset.x = -3;
		SetLowColor(ViewColor());
		DrawString(septext.String(), f_dayrect.RightBottom() -offset);
	}
			
	//draw year
	if (updaterect.Contains(f_yearrect)) {
		if (f_focus == FOCUS_YEAR && IsFocus()) {
			SetLowColor(dark);
			value = f_holdvalue;
		} else {
			SetLowColor(ViewColor());
			value = f_year;
		}
	
		text = "";
		text << value +1900;
		width = be_plain_font->StringWidth(text.String());
		offset.x = -(f_yearrect.Width()/2.0 -width/2.0);
		drawpt = f_yearrect.LeftBottom() -offset;
		
		FillRect(f_yearrect, B_SOLID_LOW);
		DrawString(text.String(), drawpt);
	}
}


void
TDateEdit::SetFocus(bool forward)
{
	switch(f_focus) {
		case FOCUS_MONTH:
			(forward)?f_focus = FOCUS_DAY: f_focus = FOCUS_YEAR;
			break;
		case FOCUS_DAY:
			(forward)?f_focus = FOCUS_YEAR: f_focus = FOCUS_MONTH;
			break;
		case FOCUS_YEAR:
			(forward)?f_focus = FOCUS_MONTH: f_focus = FOCUS_DAY;
			break;
		default:
			break;
	}
	
	UpdateFocus();
}


void
TDateEdit::UpdateFocus(bool sethold = false)
{
	switch(f_focus) {
		case FOCUS_MONTH:
		{
			if (sethold)
				f_holdvalue = f_month;
			else
			{
				if (f_holdvalue> 11) 
					f_holdvalue = 0;
				else 
				if (f_holdvalue < 0)
					f_holdvalue = 11;
				
				f_month = f_holdvalue;
			}
			Draw(f_monthrect);
		}
		break;
		case FOCUS_DAY:
		{
			if (sethold)
				f_holdvalue = f_day;
			else
			{
				int daycnt = getDaysInMonth(f_month, f_year);
				if (f_holdvalue> daycnt)
					f_holdvalue = 1;
				else
				if (f_holdvalue < 0)
					f_holdvalue = daycnt;
				f_day = f_holdvalue;
			}
			Draw(f_dayrect);
		}
		break;
		case FOCUS_YEAR: 
		{
			if (sethold)
				f_holdvalue = f_year;
			else
			{
				if (f_holdvalue> YEAR_DELTA)
					f_holdvalue = 65;
				else
				if (f_holdvalue < 65)
					f_holdvalue = YEAR_DELTA;
				f_year = f_holdvalue;
			}
			Draw(f_yearrect);
		}
		break;
		default:
			break;
	}
}


void
TDateEdit::KeyDown(const char *bytes, int32 numbytes)
{
	switch(bytes[0]) {
		case B_LEFT_ARROW:
			SetFocus(false);
		break;
		
		case B_RIGHT_ARROW:
			SetFocus(true);
		break;
		
		case B_UP_ARROW:
			UpPress();
		break;
		
		case B_DOWN_ARROW:
			DownPress();
		break;
		
		default:
			BView::KeyDown(bytes, numbytes);
		break;
	}
}


void
TDateEdit::UpPress()
{
	f_holdvalue += 1;
	UpdateFocus();
	DispatchMessage();
}


void
TDateEdit::DownPress()
{
	f_holdvalue -= 1;
	UpdateFocus();
	DispatchMessage();
}


void
TDateEdit::SetTo(int32 year, int32 month, int32 day)
{
	if (f_month != month
	 || f_day != day
	 || f_year != year)
	{
		f_month = month;
		f_day = day;
		f_year = year;
		Draw(Bounds());
	}
}

void
TDateEdit::BuildDispatch(BMessage *message)
{
	message->AddBool("time", false);
	message->AddInt32("month", f_month);
	message->AddInt32("day", f_day);
	message->AddInt32("year", f_year);
}
