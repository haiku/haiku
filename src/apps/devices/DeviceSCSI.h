/*
 * Copyright 2008-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef DEVICESCSI_H
#define DEVICESCSI_H


#include "Device.h"


class DeviceSCSI : public Device {
public:
						DeviceSCSI(Device* parent);
	virtual				~DeviceSCSI();
	virtual void		InitFromAttributes();
};

#endif /* DEVICESCSI_H */
