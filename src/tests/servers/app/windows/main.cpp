// main.cpp

#include <stdio.h>

#include "Application.h"
#include "View.h"
#include "Window.h"

// main
int
main(int argc, char** argv)
{
	BApplication* app = new BApplication("application/x.vnd-Haiku.windows_test");

	BRect bounds(50.0, 50.0, 200.0, 150.0);

	BWindow* window = new BWindow(bounds, "Test",
								  B_TITLED_WINDOW,
								  B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);

	BView* view = new BView(window->Bounds(), "test", B_FOLLOW_ALL, B_WILL_DRAW);

	window->AddChild(view);

	window->Show();
	app->Run();
	delete app;
	return 0;
}
