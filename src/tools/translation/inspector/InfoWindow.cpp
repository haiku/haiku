/*****************************************************************************/
// InfoWindow
// Written by Michael Wilber, Haiku Translation Kit Team
//
// InfoWindow.cpp
//
// BWindow class for displaying information about the currently open
// document
//
//
// Copyright (c) 2003 Haiku Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#include "Constants.h"
#include "InfoWindow.h"
#include <Application.h>
#include <ScrollView.h>
#include <Message.h>
#include <String.h>

InfoWindow::InfoWindow(BRect rect, const char *name, const char *text)
	: BWindow(rect, name, B_DOCUMENT_WINDOW, 0)
{
	BRect rctframe = Bounds();
	rctframe.right -= B_V_SCROLL_BAR_WIDTH;
	rctframe.bottom -= B_H_SCROLL_BAR_HEIGHT;

	BRect rcttext = rctframe;
	rcttext.OffsetTo(B_ORIGIN);

	fptextView = new BTextView(rctframe, "infoview", rcttext,
		B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED);

	BScrollView *psv;
	AddChild(psv = new BScrollView("infoscrollview", fptextView,
				B_FOLLOW_ALL_SIDES, 0, true, true));
	fptextView->MakeEditable(false);
	//fptextView->MakeResizable(true);
	fptextView->SetText(text);

	SetSizeLimits(100, 10000, 100, 10000);

	Show();
}

InfoWindow::~InfoWindow()
{
}

//
// TextWindow::FrameResized
//
// Adjust the size of the BTextView's text rectangle
// when the window is resized.
//
void
InfoWindow::FrameResized(float width, float height)
{
	BRect rcttext = fptextView->TextRect();

	rcttext.right = rcttext.left + (width - B_V_SCROLL_BAR_WIDTH);
	fptextView->SetTextRect(rcttext);
}

void
InfoWindow::MessageReceived(BMessage *pmsg)
{
	switch (pmsg->what) {
		case M_INFO_WINDOW_TEXT:
		{
			// App is telling me new info has arrived;
			// The view text must be updated
			BString bstr;
			if (pmsg->FindString("text", &bstr) == B_OK)
				fptextView->SetText(bstr.String());
			break;
		}

		default:
			BWindow::MessageReceived(pmsg);
			break;
	}
}

void
InfoWindow::Quit()
{
	// tell the app to forget about this window
	be_app->PostMessage(M_INFO_WINDOW_QUIT);
	BWindow::Quit();
}
