/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#ifndef INPUT_WINDOW_H
#define INPUT_WINDOW_H


#include <Box.h>
#include <CardView.h>
#include <Input.h>
#include <ListItem.h>
#include <ListView.h>
#include <Message.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <View.h>
#include <Window.h>

#include "InputDeviceView.h"
#include "InputKeyboard.h"
#include "InputMouse.h"
#include "InputTouchpadPrefView.h"
#include "MouseSettings.h"
#include "touchpad_settings.h"


class BSplitView;
class BCardView;
class BCardLayout;

class SettingsView;
class DeviceName;
class InputDevices;
class InputKeyboard;
class InputMouse;
class MultipleMouseSettings;
class TouchpadPrefView;
class TouchpadPref;
class TouchpadView;


class InputWindow : public BWindow
{
public:
							InputWindow(BRect rect);
		void				MessageReceived(BMessage* message);
		void				Show();
		void				Hide();

private:
		status_t			FindDevice();
		void				AddDevice(BInputDevice* device);

private:
		BListView*			fDeviceListView;
		BCardView*			fCardView;

		MultipleMouseSettings 	fMultipleMouseSettings;
};

#endif /* INPUT_WINDOW_H */
