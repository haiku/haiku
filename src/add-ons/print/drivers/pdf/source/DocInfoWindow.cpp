/*

DocInfoWindow.cpp

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

#include <InterfaceKit.h>
#include <SupportKit.h>
#include "DocInfoWindow.h"
#include <ctype.h>


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


// --------------------------------------------------
DocInfoWindow::DocInfoWindow(BMessage *doc_info)
	:	HWindow(BRect(0,0,400,220), "Document Information", B_TITLED_WINDOW_LOOK,
 			B_MODAL_APP_WINDOW_FEEL, B_NOT_RESIZABLE | B_NOT_MINIMIZABLE |
 			B_NOT_ZOOMABLE)
{
	// ---- Ok, build a default page setup user interface
	BRect		r;
	BBox		*panel;
	BButton		*button;
	float		x, y, w, h;
	BString 	setting_value;

	fDocInfo = doc_info;
	
	// add a *dialog* background
	r = Bounds();
	panel = new BBox(r, "top_panel", B_FOLLOW_ALL, 
					B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
					B_PLAIN_BORDER);

	// add list of keys
	fKeyList = new BMenu("Delete Key");
	BMenuField *menu = new BMenuField(BRect(0, 0, 90, 10), "delete", "", fKeyList);
	menu->SetDivider(0);
	panel->AddChild(menu);

	// add table for text controls (document info key and value)
	fTable = new Table(BRect(r.left+5, r.top+5, r.right-5-B_V_SCROLL_BAR_WIDTH, r.bottom-5-B_H_SCROLL_BAR_HEIGHT-30-20), "table", B_FOLLOW_ALL, B_WILL_DRAW);
	fTable->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// add table to ScrollView
	fTableScrollView = new BScrollView("scroll_table", fTable, B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true);
	panel->AddChild(fTableScrollView);

	// position list of keys
	menu->MoveTo(5, fTableScrollView->Frame().bottom+2);

	BMenu* defaultKeys = new BMenu("Default Keys");
	defaultKeys->AddItem(new BMenuItem("Title",        new BMessage(DEFAULT_KEY_MSG)));
	defaultKeys->AddItem(new BMenuItem("Author",       new BMessage(DEFAULT_KEY_MSG)));
	defaultKeys->AddItem(new BMenuItem("Subject",      new BMessage(DEFAULT_KEY_MSG)));
	defaultKeys->AddItem(new BMenuItem("Keywords",     new BMessage(DEFAULT_KEY_MSG)));
	defaultKeys->AddItem(new BMenuItem("Creator",      new BMessage(DEFAULT_KEY_MSG)));
	// PDFlib sets these itselves:
	// defaultKeys->AddItem(new BMenuItem("Producer",     new BMessage(DEFAULT_KEY_MSG)));
	// defaultKeys->AddItem(new BMenuItem("CreationDate", new BMessage(DEFAULT_KEY_MSG)));
	// Not meaningful to set the modification date at creation time!
	// defaultKeys->AddItem(new BMenuItem("ModDate",      new BMessage(DEFAULT_KEY_MSG)));
	BMenuField *keys = new BMenuField(BRect(0, 0, 90, 10), "add", "", defaultKeys);
	keys->SetDivider(0);
	panel->AddChild(keys);
	keys->MoveTo(menu->Frame().right + 5, menu->Frame().top);

	// add add key text control
	BTextControl *add = new BTextControl(BRect(0, 0, 180, 20), "add", "Add Key:", "", new BMessage(ADD_KEY_MSG));
	add->SetDivider(60);
	panel->AddChild(add);
	add->MoveTo(keys->Frame().right + 5, keys->Frame().top);

	// fill table
	BuildTable(fDocInfo);

	// add a "OK" button, and make it default
	button 	= new BButton(r, NULL, "OK", new BMessage(OK_MSG), 
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	button->ResizeToPreferred();
	button->GetPreferredSize(&w, &h);
	x = r.right - w - 8;
	y = r.bottom - h - 8;
	button->MoveTo(x, y);
	panel->AddChild(button);

	// add a "Cancel button	
	button 	= new BButton(r, NULL, "Cancel", new BMessage(CANCEL_MSG), 
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	button->GetPreferredSize(&w, &h);
	button->ResizeToPreferred();
	button->MoveTo(x - w - 8, y);
	panel->AddChild(button);

	// add a separator line...
	BBox * line = new BBox(BRect(r.left, y - 9, r.right, y - 8), NULL,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM );
	panel->AddChild(line);

	// Finally, add our panel to window
	AddChild(panel);
	
	MoveTo(320, 320);
	
	if (fTable->ChildAt(0)) fTable->ChildAt(0)->MakeFocus();
}


// --------------------------------------------------
bool 
DocInfoWindow::QuitRequested()
{
	return true;
}


// --------------------------------------------------
void 
DocInfoWindow::Quit()
{
	EmptyKeyList();
	inherited::Quit();
}


// --------------------------------------------------
void 
DocInfoWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what){
		case OK_MSG: ReadFieldsFromTable(fDocInfo); Quit();
			break;
		
		case CANCEL_MSG: Quit();
			break;
			
		case DEFAULT_KEY_MSG:
		case ADD_KEY_MSG: AddKey(msg, msg->what == ADD_KEY_MSG);
			break;
		
		case REMOVE_KEY_MSG: RemoveKey(msg);
			break;

		default:
			inherited::MessageReceived(msg);
			break;
	}
}

// --------------------------------------------------
void 
DocInfoWindow::BuildTable(BMessage *docInfo) 
{
	BRect r;
	float y;
	float w;
	float rowHeight;
#ifndef B_BEOS_VERSION_DANO	
	char *name;
#else
	const char *name;
#endif		
	uint32 type;
	int32 count;

	EmptyKeyList();
	while (fTable->ChildAt(0)) {
		BView *child = fTable->ChildAt(0);
		fTable->RemoveChild(child);
		delete child;
	}

	fTable->ScrollTo(0, 0);
	
	r = fTable->Bounds();
	y = 5;
	w = r.Width() - 10;

	for (int32 i = 0; docInfo->GetInfo(B_STRING_TYPE, i, &name, &type, &count) != B_BAD_INDEX; i++) {
		if (type == B_STRING_TYPE) {
			BString value;
			if (docInfo->FindString(name, &value) == B_OK) {
				BString s;
				TextControl* v = new TextControl(BRect(0, 0, w, 20), name, name, value.String(), new BMessage(), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
				float w;
				fTable->AddChild(v);
				v->GetPreferredSize(&w, &rowHeight);
				v->MoveTo(5, y);
				y += rowHeight + 2;

				fKeyList->AddItem(new BMenuItem(name, new BMessage(REMOVE_KEY_MSG)));
			}
		}
	}
	
	BScrollBar *sb = fTableScrollView->ScrollBar(B_VERTICAL);
	if (sb) {
		float th = fTable->Bounds().Height()+1;
		float h = y - th;
		if (h > 0) {
			sb->SetProportion(th / (float)y);
			sb->SetRange(0, h);
			sb->SetSteps(rowHeight + 2 + 2, th);
		} else {
			sb->SetRange(0, 0);
		}
	}
}


// --------------------------------------------------
void 
DocInfoWindow::ReadFieldsFromTable(BMessage *toDocInfo) 
{
	BView* child;
	BMessage m;
	for (int32 i = 0; (child = fTable->ChildAt(i)) != NULL; i++) {
		TextControl* t = dynamic_cast<TextControl*>(child);
		if (t) {
			m.AddString(t->Label(), t->Text());
		}
	}
	*toDocInfo = m;
}


// --------------------------------------------------
bool 
DocInfoWindow::IsValidKey(const char* key) 
{
	if (*key == 0) return false;
	while (*key) {
		if (isspace(*key) || iscntrl(*key)) break;
		key ++;
	}
	return *key == 0;
}


// --------------------------------------------------
void 
DocInfoWindow::AddKey(BMessage *msg, bool textControl) 
{
	void         *p;
	BTextControl *text;
	BMenuItem    *item;
	BString      key;
	BMessage     docInfo;
	if (msg->FindPointer("source", &p) != B_OK || p == NULL) return;
	if (textControl) {
		text = reinterpret_cast<BTextControl*>(p);
		key = text->Text();
	} else {
		item = reinterpret_cast<BMenuItem*>(p);
		key = item->Label();
	}
	
	// key is valid and is not in list already
	if (IsValidKey(key.String())) {
		BMessage docInfo;
		ReadFieldsFromTable(&docInfo);
		if (!docInfo.HasString(key.String())) {
			docInfo.AddString(key.String(), "");
			BuildTable(&docInfo);
		}
	}
}


// --------------------------------------------------
void 
DocInfoWindow::RemoveKey(BMessage *msg) 
{
	void *p;
	BMenuItem *item;
	BString key;
	BMessage docInfo;
	if (msg->FindPointer("source", &p) != B_OK) return;
	item = reinterpret_cast<BMenuItem*>(p);
	if (!item) return;
	key = item->Label();
	
	ReadFieldsFromTable(&docInfo);
	if (docInfo.HasString(key.String())) {
		docInfo.RemoveName(key.String());
		BuildTable(&docInfo);
	}
}

// --------------------------------------------------
void 
DocInfoWindow::EmptyKeyList() 
{
	while (fKeyList->ItemAt(0)) {
		BMenuItem *i = fKeyList->ItemAt(0);
		fKeyList->RemoveItem(i);
		delete i;
	}
}
