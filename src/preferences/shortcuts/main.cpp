/*
 * Copyright 1999-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */


#include <Catalog.h>
#include <Locale.h>

#include "KeyInfos.h"
#include "ShortcutsApp.h"


int
main(int argc, char** argv)
{
	InitKeyIndices();
	ShortcutsApp app;
	
	app.Run();
}

