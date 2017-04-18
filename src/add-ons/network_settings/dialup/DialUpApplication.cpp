/* -----------------------------------------------------------------------
 * Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
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

#include <Application.h>
#include <Window.h>

#include "InterfaceUtils.h"

#include "DialUpView.h"


static const char *kSignature = "application/x-vnd.haiku.dial-up-preflet";


class DialUpApplication : public BApplication {
	public:
		DialUpApplication();
};


class DialUpWindow : public BWindow {
	public:
		DialUpWindow(BRect frame);
		
		virtual bool QuitRequested()
			{ be_app->PostMessage(B_QUIT_REQUESTED); return true; }
};


int main()
{
	new DialUpApplication();
	
	be_app->Run();
	
	delete be_app;
	
	return 0;
}


DialUpApplication::DialUpApplication()
	: BApplication(kSignature)
{
	BRect rect(150, 50, 450, 435);
	DialUpWindow *window = new DialUpWindow(rect);
	window->MoveTo(center_on_screen(rect, window));
	window->Show();
}


DialUpWindow::DialUpWindow(BRect frame)
	: BWindow(frame, "DialUp", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	DialUpView *view = new DialUpView(Bounds());
	
	AddChild(view);
}
