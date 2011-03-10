/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>

#include "ConfigWindow.h"


int
main(int argc, char** argv)
{
	BApplication app("application/x-vnd.Haiku-Mail");

	BWindow* window = new ConfigWindow;
	window->Show();

	app.Run();
	return 0;
}
