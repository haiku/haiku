/*****************************************************************************/
// TranslatorWindow
// Written by Michael Wilber, Haiku Translation Kit Team
//
// TranslatorWindow.h
//
// This BWindow based object is used to hold a Translator's BView object when
// the user runs the Translator as an application.
//
//
// Copyright (c) 2004 Haiku Project
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

#ifndef TRANSLATORWINDOW_H
#define TRANSLATORWINDOW_H

#include <Application.h>
#include <Catalog.h>
#include <Locale.h>
#include <Rect.h>
#include <Translator.h>
#include <View.h>
#include <Window.h>


class TranslatorWindow : public BWindow {
public:
	TranslatorWindow(BRect area, const char *title);
		// Sets up a BWindow with bounds area
		
	~TranslatorWindow();
		// Posts a quit message so that the application closes properly
};


status_t
LaunchTranslatorWindow(BTranslator *translator, const char *title,
						BRect rect = BRect(0, 0, 1, 1));

#endif // #ifndef TRANSLATORWINDOW_H

