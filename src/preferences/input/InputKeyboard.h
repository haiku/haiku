/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#ifndef INPUT_KEYBOARD_H
#define INPUT_KEYBOARD_H

#include <Button.h>
#include <Window.h>

#include "InputDeviceView.h"
#include "KeyboardSettings.h"
#include "KeyboardView.h"

class DeviceListView;

class InputKeyboard : public BView
{
public:
			InputKeyboard(BInputDevice* dev);

	void	MessageReceived(BMessage* message);
private:
	KeyboardView		*fSettingsView;
	KeyboardSettings	fSettings;
	BButton*			fDefaultsButton;
	BButton*			fRevertButton;
};

#endif
