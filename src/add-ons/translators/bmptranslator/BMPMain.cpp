/*****************************************************************************/
// BMPTranslator
//
// Version: 1.0.0 Beta
//
// This translator opens and writes BMP files.
//
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2002 OpenBeOS Project
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
#include "BMPTranslator.h"
#include "BMPWindow.h"
#include "BMPView.h"

// ---------------------------------------------------------------
// main
//
// Creates a BWindow for displaying info about the BMPTranslator
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
	BApplication app("application/x-vnd.obos-bmp-translator");
	BMPTranslator *ptranslator = new BMPTranslator;
	BView *v = NULL;
	BRect r(0,0,225,175);
	if (ptranslator->MakeConfigurationView(NULL, &v, &r)) {
		BAlert *err = new BAlert("Error", "Something is wrong with the BMPTranslator!", "OK");
		err->Go();
		return 1;
	}
	// release the translator even though I never really used it anyway
	ptranslator->Release();
	ptranslator = NULL;
		
	BMPWindow *w = new BMPWindow(r);
	v->ResizeTo(r.Width(), r.Height());
	w->AddChild(v);
	BPoint o = B_ORIGIN;
	{
		BScreen scrn;
		BRect f = scrn.Frame();
		f.InsetBy(10,23);
		/* if not in a good place, start where the cursor is */
		if (!f.Contains(o)) {
			uint32 i;
			v->GetMouse(&o, &i, false);
			o.x -= r.Width()/2;
			o.y -= r.Height()/2;
			/* clamp location to screen */
			if (o.x < f.left) o.x = f.left;
			if (o.y < f.top) o.y = f.top;
			if (o.x > f.right) o.x = f.right;
			if (o.y > f.bottom) o.y = f.bottom;
		}
	}
	w->MoveTo(o);
	w->Show();
	app.Run();

	return 0;
}