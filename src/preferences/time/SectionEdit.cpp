#include <Bitmap.h>
#include <Window.h>

#include "Bitmaps.h"
#include "SectionEdit.h"
#include "TimeMessages.h"

const uint32 kArrowAreaWidth = 16;
const uint32 kSeperatorWidth = 8;


TSection::TSection(BRect frame)
	: f_frame(frame)
{
}


void
TSection::SetBounds(BRect frame)
{
	f_frame = frame;
}


 BRect
TSection::Bounds() const
{
	BRect bounds(f_frame);
	return bounds.OffsetByCopy(B_ORIGIN);
}


 BRect
TSection::Frame() const
{
	return f_frame;
}



TSectionEdit::TSectionEdit(BRect frame, const char *name, uint32 sections)
	: BControl(frame, name, NULL, NULL, B_FOLLOW_NONE, B_NAVIGABLE|B_WILL_DRAW)
	, f_focus(-1)
{
	f_sectioncount = sections;
	InitView();
}


TSectionEdit::~TSectionEdit()
{
}


void
TSectionEdit::AttachedToWindow()
{
	if (Parent()) {
		SetViewColor(Parent()->ViewColor());
		ReplaceTransparentColor(f_up, ViewColor());
		ReplaceTransparentColor(f_down, ViewColor());
	}
}


void
TSectionEdit::Draw(BRect updaterect)
{
	DrawBorder();
	for (uint32 idx = 0; idx < f_sectioncount; idx++)
	{
		DrawSection(idx, ((uint32)f_focus == idx) && IsFocus());
		if (idx <f_sectioncount -1)
			DrawSeperator(idx);
	}
}


void
TSectionEdit::MouseDown(BPoint where)
{
	MakeFocus(true);
	if (f_uprect.Contains(where))
		DoUpPress();
	else if (f_downrect.Contains(where))
		DoDownPress();
	else if (f_sections->CountItems()> 0) {
		TSection *section;
		for (uint32 idx = 0; idx < f_sectioncount; idx++) {
			section = (TSection *)f_sections->ItemAt(idx);
			if (section->Frame().Contains(where)) {
				SectionFocus(idx);
				return;
			}
		}	
	}
}


void
TSectionEdit::KeyDown(const char *bytes, int32 numbytes)
{
	if (f_focus == -1)
		SectionFocus(0);
		
	switch (bytes[0]) {
		case B_LEFT_ARROW:
			f_focus -= 1;
			if (f_focus < 0)
				f_focus = f_sectioncount -1;
			SectionFocus(f_focus);
		break;
		
		case B_RIGHT_ARROW:
			f_focus += 1;
			if ((uint32)f_focus >= f_sectioncount)
				f_focus = 0;
			SectionFocus(f_focus);
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


void
TSectionEdit::DispatchMessage()
{
	BMessage *msg = new BMessage(H_USER_CHANGE);
	BuildDispatch(msg);
	Window()->PostMessage(msg);
}


uint32
TSectionEdit::CountSections() const
{
	return f_sections->CountItems();
}


int32
TSectionEdit::FocusIndex() const
{
	return f_focus;
}


void
TSectionEdit::InitView()
{
	// create arrow bitmaps
	
	f_up = new BBitmap(BRect(0, 0, kUpArrowWidth -1, kUpArrowHeight -1), kUpArrowColorSpace);
	f_up->SetBits(kUpArrowBits, (kUpArrowWidth) *(kUpArrowHeight+1), 0, kUpArrowColorSpace);
	
	f_down = new BBitmap(BRect(0, 0, kDownArrowWidth -1, kDownArrowHeight -2), kDownArrowColorSpace);
	f_down->SetBits(kDownArrowBits, (kDownArrowWidth) *(kDownArrowHeight), 0, kDownArrowColorSpace);
	

	// setup sections
	f_sections = new BList(f_sectioncount);
	f_sectionsarea = Bounds().InsetByCopy(2, 2);
	f_sectionsarea.right -= (kArrowAreaWidth); 
}


void
TSectionEdit::Draw3DFrame(BRect frame, bool inset)
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
TSectionEdit::DrawBorder()
{
	rgb_color bgcolor = ViewColor();
	rgb_color light = tint_color(bgcolor, B_LIGHTEN_MAX_TINT);
	rgb_color dark = tint_color(bgcolor, B_DARKEN_1_TINT);
	rgb_color darker = tint_color(bgcolor, B_DARKEN_3_TINT);

	BRect bounds(Bounds());
	SetHighColor(light);
	SetLowColor(dark);
	Draw3DFrame(bounds, true);
	StrokeLine(bounds.LeftBottom(), bounds.LeftBottom(), B_SOLID_LOW);
	
	bounds.InsetBy(1, 1);
	bounds.right -= kArrowAreaWidth;
	
	f_showfocus = (IsFocus() && Window() && Window()->IsActive());
	if (f_showfocus) {
		rgb_color navcolor = keyboard_navigation_color();
	
		SetHighColor(navcolor);
		StrokeRect(bounds);
	} else {
		// draw border thickening (erase focus)
		SetHighColor(darker);
		SetLowColor(bgcolor);	
		Draw3DFrame(bounds, false);
	}

	// draw up/down control
	SetHighColor(light);
	bounds.left = bounds.right +1;// -kArrowAreaWidth;
	bounds.right = Bounds().right -1;
	f_uprect.Set(bounds.left +3, bounds.top +2, bounds.right, bounds.bottom /2.0);
	f_downrect = f_uprect.OffsetByCopy(0, f_uprect.Height()+2);
	
	DrawBitmap(f_up, f_uprect.LeftTop());
	DrawBitmap(f_down, f_downrect.LeftTop());
	
	Draw3DFrame(bounds, false);
	SetHighColor(dark);
	StrokeLine(bounds.LeftBottom(), bounds.RightBottom());
	StrokeLine(bounds.RightBottom(), bounds.RightTop());
	SetHighColor(light);
	StrokeLine(bounds.RightTop(), bounds.RightTop()); 
}


void
TSectionEdit::SetSections(BRect)
{
}


void
TSectionEdit::GetSeperatorWidth(uint32 *width)
{
	*width = 0;
}


void
TSectionEdit::DrawSection(uint32 index, bool isfocus)
{
}


void
TSectionEdit::DrawSeperator(uint32 index)
{
}


void
TSectionEdit::SectionFocus(uint32 index)
{
}


void
TSectionEdit::SectionChange(uint32 section, uint32 value)
{
}


void
TSectionEdit::DoUpPress()
{
}


void
TSectionEdit::DoDownPress()
{
}


//---//
