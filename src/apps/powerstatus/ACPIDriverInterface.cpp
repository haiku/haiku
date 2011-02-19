/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */


#include "ACPIDriverInterface.h"

#include <stdio.h>

#include <Autolock.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>


RateBuffer::RateBuffer()
	:
	fPosition(0),
	fSize(kRateBufferSize),
	fCurrentSize(0)
{

}


void
RateBuffer::AddRate(int32 rate)
{
	fRateBuffer[fPosition] = rate;
	fPosition ++;
	if (fPosition >= fSize)
		fPosition = 0;

	if (fCurrentSize < fSize)
		fCurrentSize ++;
}


int32
RateBuffer::GetMeanRate()
{
	int mean = 0;
	for (int i = 0; i < fCurrentSize; i++) {
		mean += fRateBuffer[i];
	}

	if (fCurrentSize == 0)
		return -1;

	return mean / fCurrentSize;
}


Battery::Battery(int driverHandler)
	:
	fDriverHandler(driverHandler)
{
	_Init();
}


Battery::~Battery()
{
	close(fDriverHandler);
}


status_t
Battery::InitCheck()
{
	return fInitStatus;
}


status_t
Battery::ReadBatteryInfo()
{
	status_t status;
	status = ioctl(fDriverHandler, GET_BATTERY_INFO, &fCachedAcpiInfo,
		sizeof(acpi_battery_info));

	if (status != B_OK)
		return status;

	return B_OK;
}


status_t
Battery::GetBatteryInfoCached(battery_info* info)
{
	info->state = fCachedAcpiInfo.state;
	info->current_rate = fCachedAcpiInfo.current_rate;
	info->capacity = fCachedAcpiInfo.capacity;
	info->full_capacity = fExtendedBatteryInfo.last_full_charge;
	fRateBuffer.AddRate(fCachedAcpiInfo.current_rate);
	if (fCachedAcpiInfo.current_rate > 0)
		info->time_left = 3600 * fCachedAcpiInfo.capacity
			/ fRateBuffer.GetMeanRate();
	else
		info->time_left = -1;

	return B_OK;
}


status_t
Battery::GetExtendedBatteryInfo(acpi_extended_battery_info* info)
{
	status_t status;
	status = ioctl(fDriverHandler, GET_EXTENDED_BATTERY_INFO, info,
		sizeof(acpi_extended_battery_info));

	return status;
}


void
Battery::_Init()
{
	uint32 magicId = 0;
	fInitStatus = ioctl(fDriverHandler, IDENTIFY_DEVICE, &magicId,
		sizeof(uint32));
	if (fInitStatus != B_OK)
		return;

	fInitStatus = ioctl(fDriverHandler, GET_EXTENDED_BATTERY_INFO,
		&fExtendedBatteryInfo, sizeof(acpi_extended_battery_info));
	if (fInitStatus != B_OK)
		return;

	fInitStatus = ioctl(fDriverHandler, GET_BATTERY_INFO, &fCachedAcpiInfo,
		sizeof(acpi_battery_info));
	if (fInitStatus != B_OK)
		return;

	printf("ACPI driver found\n");

}


ACPIDriverInterface::~ACPIDriverInterface()
{
	for (int i = 0; i < fDriverList.CountItems(); i++)
		delete fDriverList.ItemAt(i);

}


const char* kDriverDir = "/dev/power";


status_t
ACPIDriverInterface::Connect()
{
	printf("ACPI connect\n");
	return _FindDrivers(kDriverDir);
}


status_t
ACPIDriverInterface::GetBatteryInfo(battery_info* info, int32 index)
{
	BAutolock autolock(fInterfaceLocker);
	if (index < 0 || index >= fDriverList.CountItems())
		return B_ERROR;

	status_t status;
	status = fDriverList.ItemAt(index)->GetBatteryInfoCached(info);
	return status;
}


status_t
ACPIDriverInterface::GetExtendedBatteryInfo(acpi_extended_battery_info* info,
	int32 index)
{
	BAutolock autolock(fInterfaceLocker);
	if (index < 0 || index >= fDriverList.CountItems())
		return B_ERROR;

	status_t status;
	status = fDriverList.ItemAt(index)->GetExtendedBatteryInfo(info);

	return status;
}


int32
ACPIDriverInterface::GetBatteryCount()
{
	return fDriverList.CountItems();
}


status_t
ACPIDriverInterface::_ReadBatteryInfo()
{
	for (int i = 0; i < fDriverList.CountItems(); i++)
		fDriverList.ItemAt(i)->ReadBatteryInfo();

	return B_OK;
}


void
ACPIDriverInterface::_WatchPowerStatus()
{
	const bigtime_t kUpdateInterval = 2000000;
		// every two seconds

	while (atomic_get(&fIsWatching) > 0) {
		_ReadBatteryInfo();
		Broadcast(kMsgUpdate);
		acquire_sem_etc(fWaitSem, 1, B_RELATIVE_TIMEOUT, kUpdateInterval);
	}
}


status_t
ACPIDriverInterface::_FindDrivers(const char* dirpath)
{
	BDirectory dir(dirpath);
	BEntry entry;

	status_t status = B_ERROR;

	while (dir.GetNextEntry(&entry) == B_OK) {
		BPath path;
		entry.GetPath(&path);

		if (entry.IsDirectory()) {
			if (_FindDrivers(path.Path()) == B_OK)
				return B_OK;
		}
		else {
			int32 handler = open(path.Path(), O_RDWR);
			if (handler >= 0) {
				printf("try %s\n", path.Path());
				Battery* battery = new Battery(handler);
				if (battery->InitCheck() == B_OK) {
					fDriverList.AddItem(battery);
					status = B_OK;
				}
				else
					delete battery;
			}
		}

	}
	return status;
}


