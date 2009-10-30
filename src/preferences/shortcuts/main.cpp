/*
 * Copyright 1999-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */


#include "ShortcutsApp.h"
#include "KeyInfos.h"

int main(int argc, char** argv)
{
	InitKeyIndices();
	(new ShortcutsApp)->Run();
	delete be_app;
}
