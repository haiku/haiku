// main.cpp

#include <stdio.h>

#include "Application.h"
#include "View.h"
#include "Window.h"

void
show_window(BRect frame, const char* name)
{
	BWindow* window = new BWindow(frame, name,
								  B_TITLED_WINDOW,
								  B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE);

	BView* view = new BView(window->Bounds(), "test", B_FOLLOW_ALL, B_WILL_DRAW);

	window->AddChild(view);

	window->Show();
}

// main
int
main(int argc, char** argv)
{
	BApplication* app = new BApplication("application/x.vnd-Haiku.windows_test");

	BRect frame(50.0, 50.0, 200.0, 150.0);
	show_window(frame, "Window #1");

	frame.Set(80.0, 100.0, 250.0, 200.0);
	show_window(frame, "Window #2");

	app->Run();

	delete app;
	return 0;
}
