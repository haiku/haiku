/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */
#ifndef APM_DRIVER_INTERFACE_H
#define APM_DRIVER_INTERFACE_H


#include "DriverInterface.h"


class APMDriverInterface : public PowerStatusDriverInterface {
public:
	virtual				~APMDriverInterface();

	virtual status_t	Connect();
	virtual status_t 	GetBatteryInfo(battery_info* info, int32 index);
	virtual status_t	GetExtendedBatteryInfo(acpi_extended_battery_info* info,
							int32 index);
	virtual int32		GetBatteryCount();

protected:
	virtual void		_WatchPowerStatus();

private:
#ifndef HAIKU_TARGET_PLATFORM_HAIKU
	int					fDevice;
#endif
};

#endif	// APM_DRIVER_INTERFACE_H
