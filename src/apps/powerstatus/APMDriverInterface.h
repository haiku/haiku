/*
 * Copyright 2009-2015, Haiku, Inc. All Rights Reserved.
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
	virtual						~APMDriverInterface();

	virtual status_t			Connect();
	virtual status_t 			GetBatteryInfo(int32 index, battery_info* info);
	virtual status_t			GetExtendedBatteryInfo(int32 index,
									acpi_extended_battery_info* info);
	virtual int32				GetBatteryCount();

protected:
	virtual void				_WatchPowerStatus();
};


#endif	// APM_DRIVER_INTERFACE_H
