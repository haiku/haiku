/*
 * Copyright 2008-2009, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes@gmail.com>
 * Copyright 2012-2013, Tri-Edge AI <triedgeai@gmail.com>
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Fredrik Modéen <fredrik_at_modeen.se>
 */

#ifndef BLUETOOTH_SETTINGS_H
#define BLUETOOTH_SETTINGS_H

#include <bluetooth/bdaddrUtils.h>
#include <bluetooth/LocalDevice.h>

#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <SettingsMessage.h>


class BluetoothSettings
{
public:
							BluetoothSettings();

			bdaddr_t		PickedDevice() const
								{ return fCurrentSettings.pickeddevice; }
			DeviceClass		LocalDeviceClass() const
								{ return fCurrentSettings.localdeviceclass; }
			int32			Policy() const
								{ return fCurrentSettings.policy; }
			int32			InquiryTime() const
								{ return fCurrentSettings.inquirytime; }

			void			SetPickedDevice(bdaddr_t pickeddevice);
			void			SetLocalDeviceClass(DeviceClass localdeviceclass);
			void			SetPolicy(int32 policy);
			void			SetInquiryTime(int32 inquirytime);

			void			LoadSettings();
			void			SaveSettings();

private:
			struct BTSetting {
				bdaddr_t pickeddevice;
				DeviceClass localdeviceclass;
				int32 policy;
				int32 inquirytime;
			};

			SettingsMessage		fSettingsMessage;

			BTSetting			fCurrentSettings;
};

#endif // BLUETOOTH_SETTINGS_H
