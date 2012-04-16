/*****************************************************************************/
// TranslatorWindow
// Written by Michael Wilber, OBOS Translation Kit Team
//
// TranslatorWindow.cpp
//
// This BWindow based object is used to hold the Translator's BView object
// when the user runs the Translator as an application.
//
//
// Copyright (c) 2004 OpenBeOS Project
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

#include <Screen.h>
#include <Alert.h>
#include <GroupLayout.h>
#include "TranslatorWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TranslatorWindow"


// ---------------------------------------------------------------
// Constructor
//
// Sets up the BWindow for holding a Translator's BView object
//
// Preconditions:
//
// Parameters: area,	The bounds of the window
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
TranslatorWindow::TranslatorWindow(BRect area, const char *title)
	:	BWindow(area, title, B_TITLED_WINDOW,
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));
		// Set the layout on layout window
		// Do nothing for a non-layout window
}


// ---------------------------------------------------------------
// Destructor
//
// Posts a quit message so that the application is close properly
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
TranslatorWindow::~TranslatorWindow()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
}


status_t
LaunchTranslatorWindow(BTranslator *translator, const char *title, BRect rect)
{
	BView *view = NULL;
	if (translator->MakeConfigurationView(NULL, &view, &rect)) {
		BAlert *err = new BAlert(B_TRANSLATE("Error"),
			B_TRANSLATE("Unable to create the view."), B_TRANSLATE("OK"));
		err->Go();
		return B_ERROR;
	}
	// release the translator even though I never really used it anyway
	translator->Release();
	translator = NULL;

	TranslatorWindow *wnd = new TranslatorWindow(rect, title);
	wnd->AddChild(view);
	BPoint wndpt = B_ORIGIN;
	{
		BScreen scrn;
		BRect frame = scrn.Frame();
		frame.InsetBy(10, 23);
		// if the point is outside of the screen frame,
		// use the mouse location to find a better point
		if (!frame.Contains(wndpt)) {
			uint32 dummy;
			view->GetMouse(&wndpt, &dummy, false);
			wndpt.x -= rect.Width() / 2;
			wndpt.y -= rect.Height() / 2;
			// clamp location to screen
			if (wndpt.x < frame.left)
				wndpt.x = frame.left;
			if (wndpt.y < frame.top)
				wndpt.y = frame.top;
			if (wndpt.x > frame.right)
				wndpt.x = frame.right;
			if (wndpt.y > frame.bottom)
				wndpt.y = frame.bottom;
		}
	}
	wnd->MoveTo(wndpt);
	wnd->Show();
	
	return B_OK;
}

