/*
 * Copyright 2019, Adrien Destugues <pulkomandy@pulkomandy.tk>
 * Distributed under terms of the MIT license.
 */

#include "DebugWindow.h"

#include <Application.h>


int main(void)
{
	BApplication app("application/x-vnd.haiku-debugwindowtest");
	DebugWindow* window = new DebugWindow("test");
	return window->Go();
}
