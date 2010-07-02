/*
 * Copyright 2002-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm (darkwyrm@earthlink.net)
 */
#include "APRMain.h"
#include "APRWindow.h"
#include <stdio.h>
#include "defs.h"

#include <Catalog.h>
#include <Locale.h>

APRApplication::APRApplication(void)
 :	BApplication(APPEARANCE_APP_SIGNATURE)
{
	fWindow = new APRWindow(BRect(100, 100, 550, 420));
	fWindow->Show();
}

int
main(int, char**)
{	
	APRApplication myApplication;
	myApplication.Run();

	return(0);
}
