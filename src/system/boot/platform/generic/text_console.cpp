/*
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/platform/generic/text_console.h>
#include <boot/vfs.h>


// This is set by the boot platform during console_init().
ConsoleNode* gConsoleNode;


void
console_clear_screen()
{
	gConsoleNode->ClearScreen();
}


int32
console_width()
{
	return gConsoleNode->Width();
}


int32
console_height()
{
	return gConsoleNode->Height();
}


void
console_set_cursor(int32 x, int32 y)
{
	gConsoleNode->SetCursor(x, y);
}


void
console_show_cursor()
{
	gConsoleNode->SetCursorVisible(true);
}


void
console_hide_cursor(void)
{
	gConsoleNode->SetCursorVisible(false);
}


void
console_set_color(int32 foreground, int32 background)
{
	gConsoleNode->SetColors(foreground, background);
}
