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

#ifndef _INTERFACE_UTILS_H
#define _INTERFACE_UTILS_H

#include <InterfaceKit.h>

// --------------------------------------------------
class HWindow : public BWindow
{
protected:
	void Init(uint32 escape_msg);
	
public:
	typedef BWindow 		inherited;

	HWindow(BRect frame, const char *title, window_type type, uint32 flags, uint32 workspace = B_CURRENT_WORKSPACE, uint32 escape_msg = B_QUIT_REQUESTED);
	HWindow(BRect frame, const char *title, window_look look, window_feel feel, uint32 flags, uint32 workspace = B_CURRENT_WORKSPACE, uint32 escape_msg = B_QUIT_REQUESTED);
	
	virtual void MessageReceived(BMessage* m);
	virtual void AboutRequested();
};

// --------------------------------------------------
class TextView : public BTextView
{
public:
	typedef BTextView inherited;
	
	TextView(BRect frame,
				const char *name,
				BRect textRect,
				uint32 rmask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
				uint32 flags = B_WILL_DRAW | B_NAVIGABLE);

	TextView(BRect frame,
				const char *name,
				BRect textRect,
				const BFont *font, const rgb_color *color,
				uint32 rmask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
				uint32 flags = B_WILL_DRAW | B_NAVIGABLE);

	void KeyDown(const char *bytes, int32 numBytes);
	void MakeFocus(bool focus = true);
	void Draw(BRect r);
};


// --------------------------------------------------
class TextControl : public BView
{
	BStringView *fLabel;
	TextView   *fText;
public:
	TextControl(BRect frame,
				const char *name,
				const char *label, 
				const char *initial_text, 
				BMessage *message,
				uint32 rmask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
				uint32 flags = B_WILL_DRAW | B_NAVIGABLE); 
	const char *Label() { return fLabel->Text(); }
	const char *Text()  { return fText->Text(); }
	void MakeFocus(bool focus = true) { fText->MakeFocus(focus); }	
	void ConvertToParent(BView* parent, BView* child, BRect &rect);
	void FocusSetTo(BView* child);
};


// --------------------------------------------------
class Table : public BView
{
public:
	typedef BView inherited;
	
	Table(BRect frame, const char *name, uint32 rmode, uint32 flags);
	void ScrollTo(BPoint p);
};

class DragListView : public BListView
{
public:
	DragListView(BRect frame, const char *name,
		list_view_type type = B_SINGLE_SELECTION_LIST,
		uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP, 
		uint32 flags = B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS);
	bool InitiateDrag(BPoint point, int32 index, bool wasSelected);
};

#endif
