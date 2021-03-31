/*
 * Copyright 2008-2009, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes@gmail.com>
 * Copyright 2012-2013, Tri-Edge AI <triedgeai@gmail.com>
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Fredrik Modéen <fredrik_at_modeen.se>
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
	Data.Policy = 0;
	Data.InquiryTime = 15;
}


void
BluetoothSettings::Load()
{
	fFile = new BFile(fPath.Path(), B_READ_ONLY);

	if (fFile->InitCheck() == B_OK) {
		fFile->Read(&Data, sizeof(Data));
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
	}

	delete fFile;
}
