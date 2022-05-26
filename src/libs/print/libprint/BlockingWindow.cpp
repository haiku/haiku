/*

InterfaceUtils.cpp

Copyright (c) 2002 Haiku.

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

#include "BlockingWindow.h"


#include <Alert.h>
#include <Debug.h>
#include <Message.h>
#include <TextView.h>


#include <string.h>


// #pragma mark -- EscapeMessageFilter


EscapeMessageFilter::EscapeMessageFilter(BWindow *window, int32 what)
	: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, '_KYD')
	, fWindow(window),
	fWhat(what)
{
}


filter_result
EscapeMessageFilter::Filter(BMessage *msg, BHandler **target)
{
	int32 key;
	filter_result result = B_DISPATCH_MESSAGE;
	if (msg->FindInt32("key", &key) == B_OK && key == 1) {
		fWindow->PostMessage(fWhat);
		result = B_SKIP_MESSAGE;
	}
	return result;
}


// #pragma mark -- HWindow


HWindow::HWindow(BRect frame, const char *title, window_type type, uint32 flags,
		uint32 workspace, uint32 escape_msg)
	: BWindow(frame, title, type, flags, workspace)
{
	Init(escape_msg);
}


HWindow::HWindow(BRect frame, const char *title, window_look look, window_feel feel,
		uint32 flags, uint32 workspace, uint32 escape_msg)
	: BWindow(frame, title, look, feel, flags, workspace)
{
	Init(escape_msg);
}


void
HWindow::Init(uint32 escape_msg)
{
	AddShortcut('i', 0, new BMessage(B_ABOUT_REQUESTED));
	AddCommonFilter(new EscapeMessageFilter(this, escape_msg));	
}


void
HWindow::MessageReceived(BMessage* msg)
{
	if (msg->what == B_ABOUT_REQUESTED) {
		AboutRequested();
	} else {
		inherited::MessageReceived(msg);
	}
}


void
HWindow::AboutRequested()
{
	const char* aboutText = AboutText();
	if (aboutText == NULL)
		return;

	BAlert *about = new BAlert("About", aboutText, "Cool");
	BTextView *v = about->TextView();
	if (v) {
		rgb_color red = {255, 0, 51, 255};
		rgb_color blue = {0, 102, 255, 255};

		v->SetStylable(true);
		char *text = (char*)v->Text();
		char *s = text;
		// set all Be in blue and red
		while ((s = strstr(s, "Be")) != NULL) {
			int32 i = s - text;
			v->SetFontAndColor(i, i+1, NULL, 0, &blue);
			v->SetFontAndColor(i+1, i+2, NULL, 0, &red);
			s += 2;
		}
		// first text line 
		s = strchr(text, '\n');
		BFont font;
		v->GetFontAndColor(0, &font);
		font.SetSize(12); // font.SetFace(B_OUTLINED_FACE);
		v->SetFontAndColor(0, s-text+1, &font, B_FONT_SIZE);
	};
	about->SetFlags(about->Flags() | B_CLOSE_ON_ESCAPE);
	about->Go();
}


// #pragma mark -- BlockingWindow


BlockingWindow::BlockingWindow(BRect frame, const char *title, window_type type,
		uint32 flags, uint32 workspace, uint32 escape_msg)
	: HWindow(frame, title, type, flags, workspace)
{
	Init(title);
}


BlockingWindow::BlockingWindow(BRect frame, const char *title, window_look look,
		window_feel feel, uint32 flags, uint32 workspace, uint32 escape_msg)
	: HWindow(frame, title, look, feel, flags, workspace)
{
	Init(title);
}


BlockingWindow::~BlockingWindow() 
{
	delete_sem(fExitSem);
}


void
BlockingWindow::Init(const char* title) 
{
	fUserQuitResult = B_OK;
	fResult = NULL;
	fExitSem = create_sem(0, title);
	fReadyToQuit = false;
}


bool
BlockingWindow::QuitRequested()
{
	if (fReadyToQuit)
		return true;

	// user requested to quit the window
	*fResult = fUserQuitResult;
	release_sem(fExitSem);
	return false;
}


void
BlockingWindow::Quit()
{
	fReadyToQuit = false; // finally allow window to quit
	inherited::Quit(); // and quit it
}


void
BlockingWindow::Quit(status_t result)
{
	if (fResult)
		*fResult = result;

	release_sem(fExitSem);
}


void
BlockingWindow::SetUserQuitResult(status_t result)
{
	fUserQuitResult = result;
}


status_t
BlockingWindow::Go()
{
	status_t result = B_ERROR;
	fResult = &result;
	Show();
	acquire_sem(fExitSem);
	// here the window still exists, because QuitRequested returns false if
	// fReadyToQuit is false, now we can quit the window and am sure that the
	// window thread dies before this thread
	if (Lock()) {
		Quit();
	} else {
		ASSERT(false); // should not reach here!!!
	}
	// here the window does not exist, good to have the result in a local variable
	return result;
}
