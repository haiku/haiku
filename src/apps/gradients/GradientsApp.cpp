/*
 * Copyright (c) 2008-2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Artur Wyszynski <harakash@gmail.com>
 */


#include "GradientsApp.h"


#include "GradientsWindow.h"


GradientsApp::GradientsApp(void)
	: BApplication("application/x-vnd.Haiku-Gradients")
{
	GradientsWindow* window = new GradientsWindow();
	window->Show();
}


int
main()
{
	GradientsApp app;
	app.Run();
	return 0;
}
