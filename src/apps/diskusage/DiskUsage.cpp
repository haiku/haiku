/*
 * Copyright (c) 1998-2007 Matthijs Hollemans
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "App.h"

#include <TrackerAddOnAppLaunch.h>

#include <stdio.h>

#include <Catalog.h>

#include "DiskUsage.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DiskUsage"


int
main() 
{
	App app;
	app.Run();
	return 0;
}

