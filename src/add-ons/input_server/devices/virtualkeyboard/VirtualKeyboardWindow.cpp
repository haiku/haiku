/*
 * Copyright 2014 Freeman Lou <freemanlou2430@yahoo.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "VirtualKeyboardWindow.h"

#include <Screen.h>

#include "KeyboardLayoutView.h"
#include "KeymapListItem.h"
#include "Keymap.h"

VirtualKeyboardWindow::VirtualKeyboardWindow()
	:
	BWindow(BRect(0,0,0,0),"Virtual Keyboard",
	B_NO_BORDER_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL,
	B_NOT_RESIZABLE | B_WILL_ACCEPT_FIRST_CLICK)
{
	BScreen screen;
	BRect screenRect(screen.Frame());
	ResizeTo(screenRect.Width(), screenRect.Height()/3);
	MoveTo(0,screenRect.Height()-screenRect.Height()/3);

	Keymap keymap;
	fKeyboardView = new KeyboardLayoutView("Keyboard");
	fKeyboardView->SetKeymap(&keymap);
	AddChild(fKeyboardView);
}


void
VirtualKeyboardWindow::MessageReceived(BMessage* message)
{
	
}
