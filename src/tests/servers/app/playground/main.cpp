#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Catalog.h>
#include <Message.h>

#include "ObjectWindow.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Playground"


// main
int
main(int argc, char** argv)
{
	BApplication* app = new BApplication("application/x-vnd.Haiku-Playground");

	BRect frame(50.0, 50.0, 600.0, 400.0);
	BWindow* window = new ObjectWindow(frame, B_TRANSLATE_SYSTEM_NAME("Playground"));

	window->Show();

	app->Run();

	delete app;
	return 0;
}
