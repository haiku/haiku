#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Message.h>

#include "ObjectWindow.h"

// main
int
main(int argc, char** argv)
{
	BApplication* app = new BApplication("application/x-vnd.Haiku-Playground");

	BRect frame(50.0, 50.0, 600.0, 400.0);
	BWindow* window = new ObjectWindow(frame, "Playground");

	window->Show();

	app->Run();

	delete app;
	return 0;
}
