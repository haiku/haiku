/*****************************************************************************/
// STXTWindow
// Written by Michael Wilber, OBOS Translation Kit Team
//
// STXTWindow.cpp
//
// This BWindow based object is used to hold the STXTView object when the
// user runs the STXTTranslator as an application.
//
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

#include "STXTWindow.h"

// ---------------------------------------------------------------
// Constructor
//
// Sets up the BWindow for holding a STXTView
//
// Preconditions:
//
// Parameters: area,	The bounds of the window
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
STXTWindow::STXTWindow(BRect area)
	:	BWindow(area, "STXTTranslator", B_TITLED_WINDOW,
			B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
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
STXTWindow::~STXTWindow()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
}
