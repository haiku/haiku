/*
 * Copyright 2008-2009, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes@gmail.com>
 * Copyright 2012-2013, Tri-Edge AI <triedgeai@gmail.com>
 *
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef BLUETOOTH_SETTINGS_H
#define BLUETOOTH_SETTINGS_H

#include <bluetooth/bdaddrUtils.h>
#include <bluetooth/LocalDevice.h>

#include <File.h>
#include <FindDirectory.h>
#include <Path.h>

class BluetoothSettings
{
public:
	struct {
		bdaddr_t 			PickedDevice;
		DeviceClass			LocalDeviceClass;
	} Data;

							BluetoothSettings();
							~BluetoothSettings();

	void 					Defaults();
	void 					Load();
	void 					Save();

private:
	BPath 					fPath;
	BFile* 					fFile;
};

#endif // BLUETOOTH_SETTINGS_H
