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

#include <SettingsMessage.h>


BluetoothSettings::BluetoothSettings()
	:
	fSettingsMessage(B_USER_SETTINGS_DIRECTORY, "Bluetooth_settings")
{
	fCurrentSettings.pickeddevice = bdaddrUtils::NullAddress();
	fCurrentSettings.localdeviceclass = DeviceClass();
	fCurrentSettings.policy = 0;
	fCurrentSettings.inquirytime = 15;
}


void
BluetoothSettings::SetPickedDevice(bdaddr_t pickeddevice)
{
	fCurrentSettings.pickeddevice = pickeddevice;
}


void
BluetoothSettings::SetLocalDeviceClass(DeviceClass localdeviceclass)
{
	fCurrentSettings.localdeviceclass = localdeviceclass;
}


void
BluetoothSettings::SetPolicy(int32 policy)
{
	fCurrentSettings.policy = policy;
}


void
BluetoothSettings::SetInquiryTime(int32 inquirytime)
{
	fCurrentSettings.inquirytime = inquirytime;
}


void
BluetoothSettings::LoadSettings()
{
	bdaddr_t* addr;
	ssize_t size;
	status_t status = fSettingsMessage.FindData("BDAddress", B_RAW_TYPE,
		(const void**)&addr, &size);
	if (status == B_OK)
		SetPickedDevice(*addr);
	else
		SetPickedDevice(bdaddrUtils::NullAddress());

	DeviceClass* devclass;
	status = fSettingsMessage.FindData("DeviceClass", B_RAW_TYPE,
		(const void**)&devclass, &size);
	if (status == B_OK)
		SetLocalDeviceClass(*devclass);
	else
		SetLocalDeviceClass(DeviceClass());

	SetPolicy(fSettingsMessage.GetValue("Policy", (int32)0));
	SetInquiryTime(fSettingsMessage.GetValue("InquiryTime", (int32)15));
}


void
BluetoothSettings::SaveSettings()
{
	fSettingsMessage.SetValue("DeviceClass", B_RAW_TYPE,
		&fCurrentSettings.localdeviceclass, sizeof(DeviceClass));
	fSettingsMessage.SetValue("BDAddress", B_RAW_TYPE, &fCurrentSettings.pickeddevice,
		sizeof(bdaddr_t));
	fSettingsMessage.SetValue("Policy", fCurrentSettings.policy);
	fSettingsMessage.SetValue("InquiryTime", fCurrentSettings.inquirytime);

	fSettingsMessage.Save();
}
