/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2001, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

BeMail(TM), Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

// ===========================================================================
//	FindWindow.cpp
// 	Copyright 1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "FindWindow.h"
#include "Mail.h"

#include <TextView.h>
#include <Button.h>
#include <Application.h>
#include <String.h>
#include <Box.h>

#include <MDRLanguage.h>

void TextBevel(BView& view, BRect r);

// ============================================================================
void TextBevel(BView& view, BRect r)
{
	r.InsetBy(-1,-1);
	view.SetHighColor(96,96,96);
	view.MovePenTo(r.left,r.bottom);
	view.StrokeLine(BPoint(r.left,r.top));
	view.StrokeLine(BPoint(r.right,r.top));
	view.SetHighColor(216,216,216);
	view.StrokeLine(BPoint(r.right,r.bottom));
	view.StrokeLine(BPoint(r.left,r.bottom));
	r.InsetBy(-1,-1);
	view.SetHighColor(192,192,192);
	view.MovePenTo(r.left,r.bottom);
	view.StrokeLine(BPoint(r.left,r.top));
	view.StrokeLine(BPoint(r.right,r.top));
	view.SetHighColor(255,255,255);
	view.StrokeLine(BPoint(r.right,r.bottom));
	view.StrokeLine(BPoint(r.left,r.bottom));
	view.SetHighColor(0,0,0);
}

//	FindWindow is modeless...

#define FINDBUTTON 'find'

static BString sPreviousFind = "";

FindWindow* FindWindow::mFindWindow = NULL;
BRect FindWindow::mLastPosition(BRect(100,300,300,374));

void FindWindow::DoFind(BWindow *window, const char *text)
{
	if (window == NULL) {
		long i=0;
		while ((bool)(window = be_app->WindowAt(i++))) {	// Send the text to a waiting window
			if (window != mFindWindow)
				if (dynamic_cast<TMailWindow *>(window) != NULL)
					break;	// Found a window
		}
	}
	
	/* ask that window who is in the front */
	window = dynamic_cast<TMailWindow *>(window)->FrontmostWindow();
	if (window == NULL)
		return;
		
//	Found a window, send a find message
	
	if (!window->Lock())
		return;
	BView *focus = window->FindView("m_content");
	window->Unlock();

	if (focus)
	{
		BMessage msg(M_FIND);
		msg.AddString("findthis",text);
		window->PostMessage(&msg, focus);
	}
}

FindPanel::FindPanel(BRect rect)
	: BBox(rect, "FindPanel", B_FOLLOW_LEFT_RIGHT,
		B_WILL_DRAW)
{
	BRect r = Bounds();
	r.InsetBy(8,8);
	r.bottom -= 44;
	BRect text = r;
	text.OffsetTo(B_ORIGIN);
	text.InsetBy(2,2);

	mBTextView = new DialogTextView(r,"BTextView",text,B_FOLLOW_ALL,B_WILL_DRAW);
	mBTextView->DisallowChar('\n');
	mBTextView->SetText(sPreviousFind.String());
	mBTextView->MakeFocus();
	AddChild(mBTextView);
	
	mFindButton = new BButton(BRect(0,0,90,20),"FINDBUTTON",
		MDR_DIALECT_CHOICE ("Find","検索"),
		new BMessage(FINDBUTTON),B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(mFindButton);
	r = mFindButton->Bounds();
	
	mFindButton->MoveTo(Bounds().right - r.Width() - 8,
		Bounds().bottom - r.Height() - 8);
	mFindButton->SetEnabled(sPreviousFind.Length());
}

FindPanel::~FindPanel()
{
	sPreviousFind = mBTextView->Text();
}

void FindPanel::AttachedToWindow()
{
	BView::AttachedToWindow();
	SetViewColor(216,216,216);
	Window()->SetDefaultButton(mFindButton);
	mFindButton->SetTarget(this);
	mBTextView->MakeFocus(true);
	mBTextView->SelectAll();
}

void FindPanel::MouseDown(BPoint point)
{
	Window()->Activate();
	BView::MouseDown(point);
}

void FindPanel::Draw(BRect)
{
	TextBevel(*this,mBTextView->Frame());
}

void FindPanel::KeyDown(const char *, int32)
{
	int32 length = mBTextView->TextLength();
	bool enabled = mFindButton->IsEnabled();

	if (length > 0 && !enabled)
		mFindButton->SetEnabled(true);
	else if (length == 0 && enabled)
		mFindButton->SetEnabled(false);
}

void FindPanel::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case FINDBUTTON:
			Find();
			break;
		default:
			BView::MessageReceived(msg);
	}
}

void FindPanel::Find()
{
	mBTextView->SelectAll();
	const char *text = mBTextView->Text();
	if (text == NULL || text[0] == 0) return;

	BWindow *window = NULL;
	long i=0;
	while ((bool)(window = be_app->WindowAt(i++))) {	// Send the text to a waiting window
		if (window != FindWindow::mFindWindow)
			break;	// Found a window
	}

	if (window)
		FindWindow::DoFind(window, text);
}

// ============================================================================


FindWindow::FindWindow()
	: BWindow(FindWindow::mLastPosition, 
		MDR_DIALECT_CHOICE ("Find","検索"),
		B_FLOATING_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_WILL_ACCEPT_FIRST_CLICK)
{
	mFindPanel = new FindPanel(Bounds());
	AddChild(mFindPanel);
	mFindWindow = this;
	Show();
}

FindWindow::~FindWindow()
{
	FindWindow::mLastPosition = Frame();
	mFindWindow = NULL;
}

void FindWindow::Find(BWindow *window)
{
	// eliminate unused parameter warning
	(void)window;

	if (mFindWindow == NULL) {
		mFindWindow = new FindWindow();
	} else
		mFindWindow->Activate();
}

void FindWindow::FindAgain(BWindow *window)
{
	if (mFindWindow) {
		mFindWindow->Lock();
		mFindWindow->mFindPanel->Find();
		mFindWindow->Unlock();
	} else if (sPreviousFind.Length() != 0)
		DoFind(window, sPreviousFind.String());
	else
		Find(window);
}

void FindWindow::SetFindString(const char *string)
{
	sPreviousFind = string;
}

const char *FindWindow::GetFindString()
{
	return sPreviousFind.String();
}


DialogTextView::DialogTextView(BRect frame, const char *name, BRect textRect,
		uint32 resizingMode, uint32 flags)
	: BTextView(frame, name, textRect, resizingMode, flags)
{
}

void DialogTextView::KeyDown(const char *bytes, int32 numBytes)
{
	BTextView::KeyDown(bytes, numBytes);
	if (Parent())
		Parent()->KeyDown(bytes, numBytes);
}

void DialogTextView::MouseDown(BPoint point)
{
	Window()->Activate();
	BTextView::MouseDown(point);
}

void DialogTextView::InsertText(const char *inText, int32 inLength, int32 inOffset,
	const text_run_array *inRuns)
{
	BTextView::InsertText(inText, inLength, inOffset, inRuns);
	if (Parent())
		Parent()->KeyDown(NULL, 0);
}

void DialogTextView::DeleteText(int32 fromOffset, int32 toOffset)
{
	BTextView::DeleteText(fromOffset, toOffset);
	if (Parent())
		Parent()->KeyDown(NULL, 0);
}
