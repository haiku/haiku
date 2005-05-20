// main.cpp

#include <Application.h>

#include "ObjectWindow.h"

// main
int
main(int argc, char** argv)
{
	BApplication* app = new BApplication("application/x.vnd-Haiku.Objects");

	BRect frame(50.0, 50.0, 450.0, 450.0);
	(new ObjectWindow(frame, "Playground"))->Show();

	app->Run();

	delete app;
	return 0;
}
