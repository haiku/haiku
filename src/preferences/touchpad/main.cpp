/*
 * Copyright 2008-2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler (haiku@Clemens-Zeidler.de)
 */


#include <Application.h>
#include <Window.h>

#include "TouchpadPrefView.h"


class TouchpadPrefWindow : public BWindow {
public:
	TouchpadPrefWindow(BRect frame, const char* title, window_type type,
			uint32 flags)
		: BWindow(frame, title, type, flags)
	{
	}

	virtual bool QuitRequested()
	{
		be_app->PostMessage(B_QUIT_REQUESTED);
		return true;
	}
};


int
main(int argc, char* argv[])
{
	BApplication* app = new BApplication("application/x-vnd.Haiku-Touchpad");
	TouchpadPrefWindow* window = new TouchpadPrefWindow(BRect(50, 50, 450, 350),
		"Touchpad", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE
			| B_ASYNCHRONOUS_CONTROLS);
	window->AddChild(
		new TouchpadPrefView(window->Bounds(), "TouchpadPrefView"));
	window->Show();
	app->Run();

	delete app;
	return 0;
}
