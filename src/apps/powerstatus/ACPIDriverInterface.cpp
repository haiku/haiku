/*
 * Copyright 2009-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */


#include "ACPIDriverInterface.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <Autolock.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>


static const char* kDriverDir = "/dev/power";


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
	int32 mean = 0;
	for (int8 i = 0; i < fCurrentSize; i++) {
		mean += fRateBuffer[i];
	}

	if (fCurrentSize == 0)
		return -1;

	return mean / fCurrentSize;
}


// #pragma mark -


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
Battery::UpdateBatteryInfo()
{
	acpi_battery_info info;
	if (ioctl(fDriverHandler, GET_BATTERY_INFO, &info,
			sizeof(acpi_battery_info)) != 0)
		return errno;

	if ((fExtendedBatteryInfo.last_full_charge > 0
			&& info.capacity > fExtendedBatteryInfo.last_full_charge)
		|| info.capacity < 0)
		return B_BAD_DATA;

	fCachedInfo = info;
	return B_OK;
}


status_t
Battery::GetBatteryInfoCached(battery_info* info)
{
	info->state = fCachedInfo.state;
	info->current_rate = fCachedInfo.current_rate;
	info->capacity = fCachedInfo.capacity;
	info->full_capacity = fExtendedBatteryInfo.last_full_charge;
	if (info->full_capacity < 0)
		info->full_capacity = fExtendedBatteryInfo.design_capacity;

	fRateBuffer.AddRate(fCachedInfo.current_rate);
	if (fCachedInfo.current_rate > 0 && fRateBuffer.GetMeanRate() != 0) {
		info->time_left = 3600 * fCachedInfo.capacity
			/ fRateBuffer.GetMeanRate();
	} else
		info->time_left = -1;

	return B_OK;
}


status_t
Battery::GetExtendedBatteryInfo(acpi_extended_battery_info* info)
{
	if (ioctl(fDriverHandler, GET_EXTENDED_BATTERY_INFO, info,
			sizeof(acpi_extended_battery_info)) != 0)
		return errno;

	return B_OK;
}


void
Battery::_Init()
{
	uint32 magicId = 0;
	if (ioctl(fDriverHandler, IDENTIFY_DEVICE, &magicId, sizeof(uint32)) != 0) {
		fInitStatus = errno;
		return;
	}
	fInitStatus = GetExtendedBatteryInfo(&fExtendedBatteryInfo);
	if (fInitStatus != B_OK)
		return;

	printf("ACPI driver found\n");
	UpdateBatteryInfo();
}


// #pragma mark - ACPIDriverInterface


ACPIDriverInterface::ACPIDriverInterface()
	:
	fInterfaceLocker("acpi interface")
{
}


ACPIDriverInterface::~ACPIDriverInterface()
{
	for (int i = 0; i < fDriverList.CountItems(); i++)
		delete fDriverList.ItemAt(i);
}


status_t
ACPIDriverInterface::Connect()
{
	return _FindDrivers(kDriverDir);
}


status_t
ACPIDriverInterface::GetBatteryInfo(int32 index, battery_info* info)
{
	BAutolock autolock(fInterfaceLocker);
	if (index < 0 || index >= fDriverList.CountItems())
		return B_ERROR;

	return fDriverList.ItemAt(index)->GetBatteryInfoCached(info);
}


status_t
ACPIDriverInterface::GetExtendedBatteryInfo(int32 index,
	acpi_extended_battery_info* info)
{
	BAutolock autolock(fInterfaceLocker);
	if (index < 0 || index >= fDriverList.CountItems())
		return B_ERROR;

	return fDriverList.ItemAt(index)->GetExtendedBatteryInfo(info);
}


int32
ACPIDriverInterface::GetBatteryCount()
{
	return fDriverList.CountItems();
}


status_t
ACPIDriverInterface::_UpdateBatteryInfo()
{
	for (int i = 0; i < fDriverList.CountItems(); i++)
		fDriverList.ItemAt(i)->UpdateBatteryInfo();

	return B_OK;
}


void
ACPIDriverInterface::_WatchPowerStatus()
{
	const bigtime_t kUpdateInterval = 2000000;
		// every two seconds

	while (atomic_get(&fIsWatching) > 0) {
		_UpdateBatteryInfo();
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
		} else {
			int32 handler = open(path.Path(), O_RDWR);
			if (handler >= 0) {
				printf("try %s\n", path.Path());
				Battery* battery = new Battery(handler);
				if (battery->InitCheck() == B_OK
					&& fDriverList.AddItem(battery)) {
					status = B_OK;
				} else
					delete battery;
			}
		}
	}
	return status;
}
