//------------------------------------------------------------------------------
//	Copyright (c) 2003-2004, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		MenuWindow.h
//	Authors:		Marc Flerackers (mflerackers@androme.be)
//					Stefano Ceccherini (burton666@libero.it)
//	Description:	BMenuWindow is the custom BWindow for menus.
//------------------------------------------------------------------------------
#ifndef __MENUWINDOW_H
#define __MENUWINDOW_H

#include <Window.h>

class BMenu;
class BMenuScroller;

class BMenuWindow : public BWindow {
public:
	BMenuWindow(const char *name);
	virtual ~BMenuWindow();
	
	void AttachMenu(BMenu *menu);
	void DetachMenu();
	
	void UpdateScrollers();

private:
	BMenuScroller *fUpperScroller;
	BMenuScroller *fLowerScroller;
};

#endif
