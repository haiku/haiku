/*

InterfaceUtils.cpp

Copyright (c) 2002 OpenBeOS. 

Author: 
	Michael Pfeiffer

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include <string.h>
#include "InterfaceUtils.h"
#include "Utils.h"

// Implementation of HWindow

// --------------------------------------------------
HWindow::HWindow(BRect frame, const char *title, window_type type, uint32 flags, uint32 workspace, uint32 escape_msg)
	: BWindow(frame, title, type, flags, workspace)
{
	Init(escape_msg);
}


// --------------------------------------------------
HWindow::HWindow(BRect frame, const char *title, window_look look, window_feel feel, uint32 flags, uint32 workspace, uint32 escape_msg)
	: BWindow(frame, title, look, feel, flags, workspace)
{
	Init(escape_msg);
}


// --------------------------------------------------
void
HWindow::Init(uint32 escape_msg)
{
	AddShortcut('i', 0, new BMessage(B_ABOUT_REQUESTED));
	AddCommonFilter(new EscapeMessageFilter(this, escape_msg));	
}


// --------------------------------------------------
void
HWindow::MessageReceived(BMessage* msg)
{
	if (msg->what == B_ABOUT_REQUESTED) {
		AboutRequested();
	} else {
		inherited::MessageReceived(msg);
	}
}

// --------------------------------------------------
static const char* 
kAbout =
"PDF Writer for BeOS\n"
"© 2001-2004 Haiku\n"
"\n"
"Philippe Houdoin\n"
"\tProject Leader\n\n"
"Simon Gauvin\n"
"\tGUI Design\n\n"
"Michael Pfeiffer\n"
"\tPDF Generation\n"
"\tConfiguration\n"
"\tInteractive Features\n"
"\t" B_UTF8_ELLIPSIS
;

void
HWindow::AboutRequested()
{
	BAlert *about = new BAlert("About PDF Writer", kAbout, "Cool");
	BTextView *v = about->TextView();
	if (v) {
		// Colors grab with Magnify on BeOS R5 deskbar logo
		rgb_color red = {203, 0, 51, 255};		
		rgb_color blue = {0, 51, 152, 255};

		v->SetStylable(true);
		char *text = (char*) v->Text();
		char *s = text;

		// set first line 20pt bold
		s = strchr(text, '\n');
		BFont bold(be_bold_font);
		bold.SetSize(20);
		v->SetFontAndColor(0, s-text, &bold);

		// set all Be in blue and red
		s = text;
		while ((s = strstr(s, "BeOS")) != NULL) {
			int32 i = s - text;
			v->SetFontAndColor(i, i+2, NULL, 0, &blue);
			v->SetFontAndColor(i+2, i+4, NULL, 0, &red);
			s += 2;
		}
		// first text line 
		s = strchr(text, '\n');
		BFont font;
		v->GetFontAndColor(0, &font);
		font.SetSize(12); // font.SetFace(B_OUTLINED_FACE);
		v->SetFontAndColor(0, s-text+1, &font, B_FONT_SIZE);
	};
	about->Go();
}


// Impelementation of TextView

// --------------------------------------------------
TextView::TextView(BRect frame,
				const char *name,
				BRect textRect,
				uint32 rmask,
				uint32 flags)
	: BTextView(frame, name, textRect, rmask, flags)
{
}


// --------------------------------------------------
TextView::TextView(BRect frame,
				const char *name,
				BRect textRect,
				const BFont *font, const rgb_color *color,
				uint32 rmask,
				uint32 flags)
	: BTextView(frame, name, textRect, font, color, rmask, flags)
{
}


// --------------------------------------------------
void 
TextView::KeyDown(const char *bytes, int32 numBytes)
{
	if (numBytes == 1 && *bytes == B_TAB) {
		BView::KeyDown(bytes, numBytes);
		return;
	}
	inherited::KeyDown(bytes, numBytes);
}


// --------------------------------------------------
void 
TextView::Draw(BRect update)
{
	inherited::Draw(update);
	if (IsFocus()) {
		// stroke focus rectangle
		SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		StrokeRect(Bounds());
	}
}


// --------------------------------------------------
void 
TextView::MakeFocus(bool focus)
{
	Invalidate();
	inherited::MakeFocus(focus);
	// notify TextControl
	BView* parent = Parent(); // BBox
	if (focus && parent) {
		parent = parent->Parent(); // TextControl
		TextControl* control = dynamic_cast<TextControl*>(parent);
		if (control) control->FocusSetTo(this);
	}
}


// Impelementation of TextControl

// --------------------------------------------------
TextControl::TextControl(BRect frame,
				const char *name,
				const char *label, 
				const char *initial_text, 
				BMessage *message,
				uint32 rmask,
				uint32 flags)
	: BView(frame, name, rmask, flags)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	BRect r(0, 0, frame.Width() / 2 -1, frame.Height());
	fLabel = new BStringView(r, "", label);
	BRect f(r);
	f.OffsetTo(frame.Width() / 2 + 1, 0);
	// box around TextView
	BBox *box = new BBox(f, "", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	f.OffsetTo(0, 0);
	f.InsetBy(1,1);
	r.InsetBy(2,2);
	fText  = new TextView(f, "", r, rmask, flags | B_NAVIGABLE);
	fText->SetWordWrap(false);
	fText->DisallowChar('\n');
	fText->Insert(initial_text);
	AddChild(fLabel); 
	AddChild(box);
	box->AddChild(fText);
}


// --------------------------------------------------
void 
TextControl::ConvertToParent(BView* parent, BView* child, BRect &rect) 
{
	do {
		child->ConvertToParent(&rect);
		child = child->Parent();
	} while (child != NULL && child != parent);
}


// --------------------------------------------------
void
TextControl::FocusSetTo(BView *child)
{
	BRect r;
	BView* parent = Parent(); // Table
	if (parent) {
		ConvertToParent(parent, child, r);
		parent->ScrollTo(0, r.top);
	}
}


// Impelementation of Implementation of Table

// --------------------------------------------------
Table::Table(BRect frame, const char *name, uint32 rmode, uint32 flags)
	: BView(frame, name, rmode, flags)
{
}


// --------------------------------------------------
void
Table::ScrollTo(BPoint p)
{
	float h = Frame().Height()+1;
	if (Parent()) {
		BScrollView* scrollView = dynamic_cast<BScrollView*>(Parent());
		if (scrollView) {
			BScrollBar *sb = scrollView->ScrollBar(B_VERTICAL);
			float min, max;
			sb->GetRange(&min, &max);
			if (p.y < (h/2)) p.y = 0;
			else if (p.y > max) p.y = max;
		}
	}
	inherited::ScrollTo(p);
}


// Impelementation of DragListView

// --------------------------------------------------
DragListView::DragListView(BRect frame, const char *name,
		list_view_type type,
		uint32 resizingMode, uint32 flags)
	: BListView(frame, name, type, resizingMode, flags)
{
}

// --------------------------------------------------
bool DragListView::InitiateDrag(BPoint point, int32 index, bool wasSelected)
{
	BMessage m;
	DragMessage(&m, ItemFrame(index), this);
	return true;
}



