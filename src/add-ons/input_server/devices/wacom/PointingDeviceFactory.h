/*
 * Copyright 2005-2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef POINTING_DEVICE_FACTORY_H
#define POINTING_DEVICE_FACTORY_H

#include <SupportDefs.h>

class MasterServerDevice;
class PointingDevice;

class PointingDeviceFactory {
 public:
	static	PointingDevice*		DeviceFor(MasterServerDevice* parent,
									const char* path);

 private:
								PointingDeviceFactory();
								~PointingDeviceFactory();
};

#endif // POINTING_DEVICE_FACTORY_H
