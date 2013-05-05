/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <Path.h>
#include <Entry.h>
#include <Alert.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#include "MainApp.h"
#include "config.h"
#include "DeviceRoster.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainApp"


MainApp *gMainApp;

bool gOverlayDisabled = false;

MainApp::MainApp()
 :
 BApplication(APP_SIG)
{
	InitPrefs();
	
	gDeviceRoster = new DeviceRoster;
	
	fMainWindow = NewWindow();
}


MainApp::~MainApp()
{
	delete gDeviceRoster;
}


status_t
MainApp::InitPrefs()
{
	return B_OK;
}


BWindow *
MainApp::NewWindow()
{
	static int i = 0;
	BRect rect(200, 200, 750, 300);
	rect.OffsetBy(i * 25, i * 25);
	i = (i + 1) % 15;
	BWindow *win = new MainWin(rect);
	win->Show();
	return win;
}


int 
main(int argc, const char *argv[])
{
	if (argc > 1)
		gOverlayDisabled = true;
	gMainApp = new MainApp;
	gMainApp->Run();
	delete gMainApp;
	return 0;
}
