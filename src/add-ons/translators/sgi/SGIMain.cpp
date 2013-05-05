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
#include <Catalog.h>
#include "SGITranslator.h"
#include "TranslatorWindow.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SGIMain"

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
	BApplication app("application/x-vnd.Haiku-SGITranslator");
	status_t result;
	result = LaunchTranslatorWindow(new SGITranslator,
		B_TRANSLATE("SGI Settings"), BRect(0, 0, 225, 175));
	if (result == B_OK) {
		app.Run();
		return 0;
	} else
		return 1;
}
