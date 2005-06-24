//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
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
//	File Name:		ScrollBarWindow.cpp
//	Author:			MrSiggler
//					DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Container window class
//  
//------------------------------------------------------------------------------
#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Box.h>
#include <Button.h>
#include <StringView.h>

#include "ScrollBarWindow.h"
#include "ScrollBarApp.h"
#include "ScrollBarView.h"

ScrollBarWindow::ScrollBarWindow(void) 
	: BWindow(	BRect(50,50,398,325), "Scroll Bar", B_TITLED_WINDOW, B_NOT_RESIZABLE | 
				B_NOT_ZOOMABLE )
{
	AddChild(new ScrollBarView);
}

ScrollBarWindow::~ScrollBarWindow(void)
{
}

bool
ScrollBarWindow::QuitRequested(void)
{
	set_scroll_bar_info(&gSettings);
	
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
