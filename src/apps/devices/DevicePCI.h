/*
 * Copyright 2008-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pieter Panman
 */
#ifndef DEVICEPCI_H
#define DEVICEPCI_H


#include "Device.h"
#include <Catalog.h>

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "DevicePCI"

class DevicePCI : public Device {
public:
						DevicePCI(Device* parent);
	virtual				~DevicePCI();
	virtual Attributes	GetBusAttributes();
	virtual BString		GetBusStrings();
	virtual void		InitFromAttributes();
	
	virtual BString		GetBusTabName()
							{ return B_TRANSLATE("PCI Information"); }

private:
	uint16				fClassBaseId;
	uint16				fClassSubId;
	uint16				fClassApiId;
	uint16				fVendorId;
	uint16				fDeviceId;
	uint16				fSubsystemVendorId;
	uint16				fSubSystemId;
};

#endif /* DEVICEPCI_H */
