/* -----------------------------------------------------------------------
 * Copyright (c) 2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 * ----------------------------------------------------------------------- */

#include "ConnectionWindow.h"


ConnectionWindow::ConnectionWindow(BRect frame, ppp_interface_id id,
		thread_id replyThread)
	: BWindow(frame, "Connecting...", B_MODAL_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	BRect bounds = Bounds();
	bounds.OffsetBy(bounds.Width() + 5, 0);
	DialUpView *dun = new DialUpView(bounds);
	fConnectionView = new ConnectionView(Bounds(), id, replyThread, dun);
	
	AddChild(dun);
	AddChild(fConnectionView);
}


bool
ConnectionWindow::QuitRequested()
{
	fConnectionView->CleanUp();
	
	return true;
}
