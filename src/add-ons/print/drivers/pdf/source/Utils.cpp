/*

PDF Writer printer driver.

Copyright (c) 2001, 2002 OpenBeOS. 

Authors: 
	Philippe Houdoin
	Simon Gauvin	
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

#include "Utils.h"
#include <Window.h>
#include <Alert.h>
#include <TextView.h>
#include <string.h>

// --------------------------------------------------
EscapeMessageFilter::EscapeMessageFilter(BWindow *window, int32 what) 
	: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, '_KYD')
	, fWindow(window), 
	fWhat(what) 
{ 
}


// --------------------------------------------------
filter_result 
EscapeMessageFilter::Filter(BMessage *msg, BHandler **target) 
{
	int32 key;
	// notify window with message fWhat if Escape key is hit
	if (B_OK == msg->FindInt32("key", &key) && key == 1) {
		fWindow->PostMessage(fWhat);
		return B_SKIP_MESSAGE;
	}
	return B_DISPATCH_MESSAGE;
}


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
"© 2001, 2002 OpenBeOS\n"
"\n"
"\tPhilippe Houdoin - Project Leader\n"
"\tSimon Gauvin - GUI Design\n"
"\tMichael Pfeiffer - PDF Generation, Configuration, Interactive Features\n"
"\tCelerick Stephens - Documentation\n"
;

void
HWindow::AboutRequested()
{
	BAlert *about = new BAlert("About PDF Writer", kAbout, "Cool");
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
	about->Go();
}


