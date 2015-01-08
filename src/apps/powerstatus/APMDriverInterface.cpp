/*
 * Copyright 2009-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */


#include "APMDriverInterface.h"

#include <arch/x86/apm_defs.h>
#include <generic_syscall_defs.h>
#include <syscalls.h>


const bigtime_t kUpdateInterval = 2000000;
		// every two seconds


APMDriverInterface::~APMDriverInterface()
{
}


status_t
APMDriverInterface::Connect()
{
	uint32 version = 0;
	status_t status = _kern_generic_syscall(APM_SYSCALLS, B_SYSCALL_INFO,
		&version, sizeof(version));
	if (status == B_OK) {
		battery_info info;
		status = _kern_generic_syscall(APM_SYSCALLS, APM_GET_BATTERY_INFO,
			&info, sizeof(battery_info));
	}

	return status;
}


status_t
APMDriverInterface::GetBatteryInfo(int32 index, battery_info* info)
{
	if (index != 0)
		return B_BAD_VALUE;

	info->current_rate = -1;

	apm_battery_info apmInfo;
	status_t status = _kern_generic_syscall(APM_SYSCALLS, APM_GET_BATTERY_INFO,
		&apmInfo, sizeof(apm_battery_info));
	if (status == B_OK) {
		info->state = apmInfo.online ? BATTERY_CHARGING : BATTERY_DISCHARGING;
		info->capacity = apmInfo.percent;
		info->full_capacity = 100;
		info->time_left = apmInfo.time_left;
	}

	return status;
}


status_t
APMDriverInterface::GetExtendedBatteryInfo(int32 index,
	acpi_extended_battery_info* info)
{
	return B_NOT_SUPPORTED;
}


int32
APMDriverInterface::GetBatteryCount()
{
	return 1;
}


void
APMDriverInterface::_WatchPowerStatus()
{
	while (atomic_get(&fIsWatching) > 0) {
		Broadcast(kMsgUpdate);
		acquire_sem_etc(fWaitSem, 1, B_RELATIVE_TIMEOUT, kUpdateInterval);
	}
}
