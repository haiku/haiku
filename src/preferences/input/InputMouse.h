/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
*/


#ifndef INPUT_MOUSE_H
#define INPUT_MOUSE_H


#include <Box.h>
#include <Bitmap.h>
#include <Button.h>
#include <CheckBox.h>
#include <Slider.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <ListView.h>
#include <TabView.h>
#include <View.h>

#include "InputDeviceView.h"
#include "MouseSettings.h"
#include "MouseView.h"
#include "SettingsView.h"

#define MOUSE_SETTINGS 'Mss'

class DeviceListView;


class InputMouse : public BView {
public:
					InputMouse(BInputDevice* dev, MouseSettings* settings);
	virtual			~InputMouse();
	void			SetMouseType(int32 type);
	void			MessageReceived(BMessage* message);
private:

	typedef BBox inherited;

	SettingsView*		fSettingsView;
	MouseView*			fMouseView;
	BButton*			fDefaultsButton;
	BButton*			fRevertButton;
	MouseSettings*		fSettings;
};

#endif	/* INPUT_MOUSE_H */
