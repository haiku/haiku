/*
 * Copyright 2008-2009, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes@gmail.com>
 * Copyright 2012-2013, Tri-Edge AI <triedgeai@gmail.com>
 *
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "BluetoothSettings.h"

BluetoothSettings::BluetoothSettings()
{
	find_directory(B_USER_SETTINGS_DIRECTORY, &fPath);
	fPath.Append("Bluetooth_settings", true);
}


BluetoothSettings::~BluetoothSettings()
{

}


void
BluetoothSettings::Defaults()
{
	Data.PickedDevice = bdaddrUtils::NullAddress();
	Data.LocalDeviceClass = DeviceClass();
}


void
BluetoothSettings::Load()
{
	fFile = new BFile(fPath.Path(), B_READ_ONLY);

	if (fFile->InitCheck() == B_OK) {
		fFile->Read(&Data, sizeof(Data));
		// TODO: Add more settings here.
	} else
		Defaults();

	delete fFile;
}


void
BluetoothSettings::Save()
{
	fFile = new BFile(fPath.Path(), B_WRITE_ONLY | B_CREATE_FILE);

	if (fFile->InitCheck() == B_OK) {
		fFile->Write(&Data, sizeof(Data));
		// TODO: Add more settings here.
	}

	delete fFile;
}
