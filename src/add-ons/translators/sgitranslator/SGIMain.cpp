/*****************************************************************************/
// SGITranslator
// Adopted by Stephan Aßmus, <stippi@yellowbites.com>
// from TIFFMain written by
// Michael Wilber, OBOS Translation Kit Team
//
// Version:
//
// This translator opens and writes SGI images.
//
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2003 OpenBeOS Project
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

#include <Application.h>
#include <Screen.h>
#include <Alert.h>
#include "SGITranslator.h"
#include "SGIWindow.h"
#include "SGIView.h"

// ---------------------------------------------------------------
// main
//
// Creates a BWindow for displaying info about the SGITranslator
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
int
main()
{
	BApplication app("application/x-vnd.obos-sgi-translator");
	SGITranslator *ptranslator = new SGITranslator;
	BView *view = NULL;
	BRect rect(0, 0, 225, 175);
	if (ptranslator->MakeConfigurationView(NULL, &view, &rect)) {
		BAlert *err = new BAlert("Error",
			"Unable to create the SGITranslator view.", "OK");
		err->Go();
		return 1;
	}
	// release the translator even though I never really used it anyway
	ptranslator->Release();
	ptranslator = NULL;

	SGIWindow *wnd = new SGIWindow(rect);
	view->ResizeTo(rect.Width(), rect.Height());
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
	app.Run();
	return 0;
}
