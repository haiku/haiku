/*****************************************************************************/
// SGIWindow
// Adopted by Stephan Aßmus, <stippi@yellowbites.com>
// from TIFFWindow written by
// Michael Wilber, OBOS Translation Kit Team
//
// SGIWindow.h
//
// This BWindow based object is used to hold the SGIView object when the
// user runs the SGITranslator as an application.
//
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

#ifndef SGIWINDOW_H
#define SGIWINDOW_H

#include <Application.h>
#include <Window.h>
#include <View.h>

class SGIWindow : public BWindow {
public:
	SGIWindow(BRect area);
		// Sets up a BWindow with bounds area
		
	~SGIWindow();
		// Posts a quit message so that the application closes properly
};

#endif // #define SGIWINDOW_H
