/*
 * Copyright 2008-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pieter Panman
 */
#ifndef DEVICEUSB_H
#define DEVICEUSB_H


#include "Device.h"


class DeviceUSB : public Device {
public:
						DeviceUSB(Device* parent);
	virtual				~DeviceUSB();
	virtual void		InitFromAttributes();

private:
	uint8				fClassBaseId;
	uint8				fClassSubId;
	uint8				fClassProtoId;
	uint16				fVendorId;
	uint16				fDeviceId;
};

#endif /* DEVICEUSB_H */
