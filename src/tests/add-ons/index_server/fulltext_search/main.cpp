/*
 * Copyright 2009-2010 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ankur Sethi (get.me.ankur@gmail.com)
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#include <Application.h>

#include "SearchWindow.h"


int main()
{
	BApplication app("application/x-vnd.Haiku-FullTextSearch");

	SearchWindow* window = new SearchWindow(BRect(50, 50, 500, 250));
	window->Show();

	app.Run();

	return 0;
}
