//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		APRMain.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Main file for Appearance
//					
//  
//------------------------------------------------------------------------------
#include "APRMain.h"
#include <stdio.h>
#include "defs.h"

APRApplication::APRApplication()
  :BApplication(APPEARANCE_APP_SIGNATURE)
{
	BRect rect;

	// This is just the size and location of the window when Show() is called	
	rect.Set(100,100,610,345);
	aprwin=new APRWindow(rect);
	aprwin->Show();
}

int main(int, char**)
{	
	APRApplication myApplication;

	// This function doesn't return until the application quits
	myApplication.Run();

	return(0);
}
