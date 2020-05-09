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
//	FindWindow.h
// 	Copyright 1996 by Peter Barrett, All rights reserved.
// ===========================================================================
#ifndef _FINDWINDOW_H
#define _FINDWINDOW_H

#include <AppDefs.h>
#include <GroupView.h>
#include <TextControl.h>
#include <Window.h>


class FindPanel;


class FindWindow : public BWindow {
friend class FindPanel;
public:
							FindWindow();
	virtual					~FindWindow();

	static	void			FindAgain(BWindow* window);
	static	void			Find(BWindow* window);
	static	bool			IsFindWindowOpen() { return fFindWindow; }
	static	void			Close() {
								if (fFindWindow)
									fFindWindow->PostMessage(B_QUIT_REQUESTED);
							}
	static	void			SetFindString(const char* string);
	static const char*		GetFindString();

protected:
	static	void			DoFind(BWindow* window, const char* text);

	static	FindWindow*		fFindWindow;
			FindPanel*		fFindPanel;
	static	BRect			fLastPosition;
};


class FindPanel : public BGroupView {
public:
							FindPanel();
	virtual					~FindPanel();
	virtual	void			AttachedToWindow();
	virtual	void			MessageReceived(BMessage* msg);
	virtual	void			Draw(BRect updateRect);
	virtual	void			KeyDown(const char* bytes, int32 numBytes);
			void			Find();
	virtual	void			MouseDown(BPoint point);

protected:
			BButton*		fFindButton;
			BTextControl*	fTextControl;
};


#endif // #ifndef _FINDWINDOW_H

