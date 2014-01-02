/*
 * Copyright 2014 Freeman Lou <freemanlou2430@yahoo.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Application.h>

#include "VirtualKeyboardWindow.h"

class VirtualKeyboardApp : public BApplication{
public:
							VirtualKeyboardApp();				
private:
	VirtualKeyboardWindow*	fWindow;
};


VirtualKeyboardApp::VirtualKeyboardApp()
	:
	BApplication("application/x-vnd.VirtualKeyboard")
{
	fWindow = new VirtualKeyboardWindow();
	fWindow->Show();
}


int
main()
{
	VirtualKeyboardApp app;
	app.Run();

	return 0;
}
