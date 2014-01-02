/*
 * Copyright 2014 Freeman Lou <freemanlou2430@yahoo.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef VIRTUAL_KEYBOARD_WINDOW_H
#define VIRTUAL_KEYBOARD_WINDOW_H

#include <Window.h>

class KeyboardLayoutView;


class VirtualKeyboardWindow : public BWindow{
public:
							VirtualKeyboardWindow();
		virtual void		MessageReceived(BMessage* message);								
private:
		KeyboardLayoutView* fKeyboardView;
};

#endif // VIRTUAL_KEYBOARD_WINDOW_H
