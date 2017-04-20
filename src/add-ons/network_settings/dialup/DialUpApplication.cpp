/*
 * Copyright 2003-2004 Waldemar Kornewald. All rights reserved.
 * Copyright 2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Application.h>
#include <Window.h>

#include "InterfaceUtils.h"

#include "DialUpView.h"


static const char *kSignature = "application/x-vnd.haiku.dial-up-preflet";


class DialUpApplication : public BApplication {
	public:
		DialUpApplication();
};


class DialUpWindow : public BWindow {
	public:
		DialUpWindow(BRect frame);

		virtual bool QuitRequested()
			{ be_app->PostMessage(B_QUIT_REQUESTED); return true; }
};


int main()
{
	new DialUpApplication();

	be_app->Run();

	delete be_app;

	return 0;
}


DialUpApplication::DialUpApplication()
	: BApplication(kSignature)
{
	BRect rect(150, 50, 450, 435);
	DialUpWindow *window = new DialUpWindow(rect);
	window->MoveTo(center_on_screen(rect, window));
	window->Show();
}


DialUpWindow::DialUpWindow(BRect frame)
	: BWindow(frame, "DialUp", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	DialUpView *view = new DialUpView(Bounds());

	AddChild(view);
}
