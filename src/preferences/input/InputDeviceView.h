/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#ifndef _INPUT_DEVICE_VIEW_H
#define _INPUT_DEVICE_VIEW_H

#include <ListView.h>
#include <ListItem.h>
#include <StringItem.h>
#include <ScrollBar.h>
#include <String.h>
#include <ScrollView.h>
#include <View.h>
#include <Message.h>

#include "InputTouchpadPref.h"
#include "MouseSettings.h"

#define ITEM_SELECTED 'I1s'

class TouchpadPref;
class MouseSettings;


class DeviceName : public BStringItem {
public:
				DeviceName(const char* item, int d);
	virtual 	~DeviceName();
	int			WhichDevice() { return fDevice; };
private:
	int fDevice;
};

class DeviceListView: public BView {
public:
			DeviceListView(const char *name);
	virtual		~DeviceListView();
	virtual	void	AttachedToWindow();
private:
	BScrollView*	fScrollView;
	BListView*	fDeviceList;
	TouchpadPref	fTouchpadPref;
	MouseSettings	fMouseSettings;
};

#endif	// _INPUT_DEVICE_VIEW_H */
