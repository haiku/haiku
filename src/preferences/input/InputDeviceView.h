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
#include <Message.h>
#include <StringItem.h>
#include <ScrollBar.h>
#include <String.h>
#include <ScrollView.h>
#include <View.h>

#include "InputTouchpadPref.h"
#include "MouseSettings.h"

#define ITEM_SELECTED 'I1s'

class TouchpadPref;
class MouseSettings;


class DeviceListView: public BView {
public:
			DeviceListView(const char *name);
	virtual		~DeviceListView();
	virtual	void	AttachedToWindow();
	BListView*	fDeviceList;

private:
	BScrollView*	fScrollView;
};

#endif	// _INPUT_DEVICE_VIEW_H */
