/*
 * Copyright 2002-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm (darkwyrm@earthlink.net)
 */
#include "Appearance.h"
#include "AppearanceWindow.h"
#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>

AppearanceApplication::AppearanceApplication(void)
 :	BApplication("application/x-vnd.Haiku-Appearance")
{
	fWindow = new AppearanceWindow(BRect(100, 100, 550, 420));
	fWindow->Show();
}

int
main(int, char**)
{	
	AppearanceApplication myApplication;
	myApplication.Run();

	return(0);
}
