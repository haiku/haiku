/*
 * Copyright 1999-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */


#include "KeyInfos.h"
#include "ShortcutsApp.h"


int main(int argc, char** argv)
{
	InitKeyIndices();
	ShortcutsApp app;
	app.Run();
}

