/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */


#include <Application.h>
#include <Catalog.h>

#include "MusicCollectionWindow.h"


const char* kAppSignature = "application/x-vnd.MusicCollection";


int
main(int argc, char* argv[])
{
	BApplication app(kAppSignature);

	BWindow* window = new MusicCollectionWindow(BRect(100, 100, 350, 500),
		B_TRANSLATE_SYSTEM_NAME("Music Collection"));
	window->Show();

	app.Run();

	return 0;
}
