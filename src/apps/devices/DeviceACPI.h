/*
 * Copyright 2008-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck (kallisti5)
 */
#ifndef DEVICEACPI_H
#define DEVICEACPI_H


#include "Device.h"


class DeviceACPI : public Device {
public:
						DeviceACPI(Device* parent);
	virtual				~DeviceACPI();
	virtual void		InitFromAttributes();
};

#endif /* DEVICEACPI_H */

