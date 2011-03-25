/*
 * Copyright 1999, Be Incorporated. All Rights Reserved.
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * This file may be used under the terms of the Be Sample Code License.
 *
 */


#include <Application.h>
#include <Catalog.h>

#include "cl_wind.h"


const char* kAppSignature = "application/x-vnd.Haiku-Clock";


int
main(int argc, char* argv[])
{
	BApplication app(kAppSignature);

	BWindow* clockWindow = new TClockWindow(BRect(100, 100, 182, 182),
		B_TRANSLATE_SYSTEM_NAME("Clock"));
	clockWindow->Show();

	app.Run();

	return 0;
}
