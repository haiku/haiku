/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#ifndef INPUT_H
#define INPUT_H


#include <Application.h>
#include <Catalog.h>
#include <Locale.h>


#include "MouseSettings.h"
#include "InputMouse.h"
#include "InputWindow.h"


class SettingsView;
class MouseSettings;

class InputApplication : public BApplication {
public:
				InputApplication();
	void		MessageReceived(BMessage* message);
private:
	InputIcons	fIcons;
	InputWindow*	fWindow;
};

#endif	/* INPUT_H */
