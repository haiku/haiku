/*
 * Copyright 2008-2009, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes@gmail.com>
 * Copyright 2012-2013, Tri-Edge AI, <triedgeai@gmail.com>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef BLUETOOTH_SETTINGS_VIEW_H
#define BLUETOOTH_SETTINGS_VIEW_H

#include "BluetoothSettings.h"

#include <View.h>

class BluetoothSettings;
class ExtendedLocalDeviceView;
class LocalDevice;

class BBox;
class BMenuField;
class BPopUpMenu;
class BSlider;

class BluetoothSettingsView : public BView {
public:
								BluetoothSettingsView(const char* name);
	virtual						~BluetoothSettingsView();

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);


private:
			void				_BuildConnectionPolicy();
			void				_BuildClassMenu();
			void				_BuildLocalDevicesMenu();
			bool				_SetDeviceClass(uint8 major, uint8 minor,
									uint16 service);
			void				_MarkLocalDevice(LocalDevice* lDevice);

protected:
			BluetoothSettings	fSettings;

			float				fDivider;

			BMenuField*			fPolicyMenuField;
			BPopUpMenu*			fPolicyMenu;
			BMenuField*			fClassMenuField;
			BPopUpMenu*			fClassMenu;
			BMenuField*			fLocalDevicesMenuField;
			BPopUpMenu*			fLocalDevicesMenu;

			ExtendedLocalDeviceView* 	fExtDeviceView;

			BSlider*			fInquiryTimeControl;

};

#endif // BLUETOOTH_SETTINGS_VIEW_H
