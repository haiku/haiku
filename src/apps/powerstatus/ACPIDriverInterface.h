/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */

#ifndef ACPI_DRIVER_INTERFACE_H
#define ACPI_DRIVER_INTERFACE_H

#include "DriverInterface.h"

#include <Locker.h>
#include <ObjectList.h>

const int8 kRateBufferSize = 10;

class RateBuffer
{
public:
							RateBuffer();
	void					AddRate(int32 rate);
	int32					GetMeanRate();
	
private:
	int32					fRateBuffer[kRateBufferSize];
	int8					fPosition;
	int8					fSize;
	int8					fCurrentSize;
};


class Battery
{
public:
								Battery(int driverHandler);
								~Battery();	

	status_t					InitCheck();

	status_t 					GetBatteryInfo(battery_info* info);
	status_t 					GetExtendedBatteryInfo(
									acpi_extended_battery_info* info);

private:
	void						_Init();

	
	int							fDriverHandler;
	status_t					fInitStatus;

	acpi_extended_battery_info	fExtendedBatteryInfo;
	
	RateBuffer					fRateBuffer;
};


class ACPIDriverInterface : public PowerStatusDriverInterface
{
public:
	virtual					~ACPIDriverInterface();

	virtual status_t		Connect();
	virtual status_t 		GetBatteryInfo(battery_info* info, int32 index);
	virtual status_t	 	GetExtendedBatteryInfo(
								acpi_extended_battery_info* info, int32 index);

	virtual int32			GetBatteryCount();

protected:
	virtual void			_WatchPowerStatus();
	virtual status_t		_FindDrivers(const char* path);

	BObjectList<Battery>	fDriverList;
	
	BLocker					fBatteryStatusLock;
};

#endif
