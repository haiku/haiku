#include<Message.h>
#include <String.h>
#include <stdio.h>

#include <math.h>

#include "TimeMessages.h"
#include "TZDisplay.h"


TTZDisplay::TTZDisplay(BRect frame, const char *name, 
		uint32 resizingmode, uint32 flags,
		const char *label, const char *text, 
		int32 hour, int32 minute, int32 second)
	: BView(frame, name, resizingmode, flags)
{
	f_label = new BString(label);
	f_text = new BString(text);
	f_time = new BString();
	
	SetTo(hour, minute, second);
}


TTZDisplay::~TTZDisplay()
{
}


void
TTZDisplay::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());
}


void
TTZDisplay::MessageReceived(BMessage *message)
{
	switch(message->what) {
		default:
			BView::MessageReceived(message);
			break;
	}
}


static float
fontheight()
{
	font_height finfo;
	be_plain_font->GetHeight(&finfo);
	float height = ceil(finfo.descent) +ceil(finfo.ascent) +ceil(finfo.leading);
	return height;
}


void
TTZDisplay::ResizeToPreferred()
{
	float height = fontheight();
	ResizeTo(Bounds().Width(), height *2);
}


void
TTZDisplay::Draw(BRect updaterect)
{
	BRect bounds(Bounds());
	SetLowColor(ViewColor());
	FillRect(bounds, B_SOLID_LOW);
	
	float height = fontheight();

	BPoint drawpt(bounds.left +2, height/2.0 +1);
	DrawString(f_label->String(), drawpt);

	drawpt.y += fontheight() +2;
	DrawString(f_text->String(), drawpt);
	
	drawpt.x = bounds.right -be_plain_font->StringWidth(f_time->String()) +2;
	DrawString(f_time->String(), drawpt);
}


void
TTZDisplay::SetLabel(const char *label)
{
	f_label->SetTo(label);
	Draw(Bounds());
}

void
TTZDisplay::SetText(const char *text)
{
	f_text->SetTo(text);
	Draw(Bounds());
}

void
TTZDisplay::SetTo(int32 hour, int32 minute, int32 second)
{
	// format time into f_time
	if (f_time == NULL)
		f_time = new BString();
	else
		 f_time->SetTo("");
	
	int32 ahour = hour;
	if (hour> 12)
		ahour = hour -12;
		
	if (ahour == 0)
		ahour = 12;

	char *ap;
	if (hour> 12)
		ap = "pm";
	else
		ap = "am";

	char *time = f_time->LockBuffer(8);
	sprintf(time, "%02lu:%02lu %s", ahour, minute, ap);
	f_time->UnlockBuffer(8);
	
	Draw(Bounds());
}
