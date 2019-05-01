/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#ifndef INPUT_WINDOW_H
#define INPUT_WINDOW_H

#include <Window.h>
#include <View.h>
#include <ListItem.h>
#include <ListView.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <Box.h>
#include <CardView.h>
#include <Message.h>

#include "MouseSettings.h"
#include "InputMouse.h"
#include "InputTouchpadPrefView.h"
#include "touchpad_settings.h"


class BSplitView;
class BCardView;
class BCardLayout;

class DeviceListView;
class SettingsView;
class DeviceName;
class TouchpadPrefView;
class TouchpadPref;
class TouchpadView;
class InputDevices;
class InputMouse;


class InputWindow : public BWindow
{
public:
							InputWindow(BRect rect);
		void				MessageReceived(BMessage* message);
private:
	DeviceListView*			fDeviceListView;
	BCardView*				fCardView;
	MouseSettings			fSettings;
	SettingsView*			fSettingsView;
	InputMouse*				fInputMouse;
	TouchpadPrefView*		fTouchpadPrefView;
	TouchpadPref*			fTouchpadPref;
};

#endif /* INPUT_WINDOW_H */
