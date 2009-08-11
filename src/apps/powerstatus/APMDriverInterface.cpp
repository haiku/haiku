/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */


#include "APMDriverInterface.h"

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
#	include <arch/x86/apm_defs.h>
#	include <generic_syscall_defs.h>
#	include <syscalls.h>
	// temporary, as long as there is no real power state API
#endif


const bigtime_t kUpdateInterval = 2000000;
		// every two seconds


#ifndef HAIKU_TARGET_PLATFORM_HAIKU
// definitions for the APM driver available for BeOS
enum {
	APM_CONTROL = B_DEVICE_OP_CODES_END + 1,
	APM_DUMP_POWER_STATUS,
	APM_BIOS_CALL,
	APM_SET_SAFETY
};

#define BIOS_APM_GET_POWER_STATUS 0x530a
#endif


APMDriverInterface::~APMDriverInterface()
{
#ifndef HAIKU_TARGET_PLATFORM_HAIKU
	close(fDevice);
#endif
}


status_t
APMDriverInterface::Connect()
{
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	uint32 version = 0;
	status_t status = _kern_generic_syscall(APM_SYSCALLS, B_SYSCALL_INFO,
		&version, sizeof(version));
	if (status == B_OK) {
		battery_info info;
		status = _kern_generic_syscall(APM_SYSCALLS, APM_GET_BATTERY_INFO,
			&info, sizeof(battery_info));
	}

	return status;

#else
	fDevice = open("/dev/misc/apm", O_RDONLY);
	if (fDevice < 0) {
		return B_ERROR;
	}

	return B_OK;
#endif
}


status_t
APMDriverInterface::GetBatteryInfo(battery_info* info, int32 index)
{
	if (index != 0)
		return B_BAD_VALUE;

	info->current_rate = -1;

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	// TODO: retrieve data from APM kernel interface
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
#else
	if (fDevice < 0)
		return B_ERROR;

	uint16 regs[6] = {0, 0, 0, 0, 0, 0};
	regs[0] = BIOS_APM_GET_POWER_STATUS;
	regs[1] = 0x1;
	if (ioctl(fDevice, APM_BIOS_CALL, regs) == 0) {
		bool online = (regs[1] >> 8) != 0 && (regs[1] >> 8) != 2;
		info->state = online ? BATTERY_CHARGING : BATTERY_DISCHARGING;
		info->capacity = regs[2] & 255;
		if (info->capacity > 100)
			info->capacity = -1;
		info->full_capacity = 100;
		info->time_left = info->capacity >= 0 ? regs[3] : -1;
		if (info->time_left > 0xffff)
			info->time_left = -1;
		else if (info->time_left & 0x8000)
			info->time_left = (info->time_left & 0x7fff) * 60;
	}

	return B_OK;
#endif
}


status_t
APMDriverInterface::GetExtendedBatteryInfo(acpi_extended_battery_info* info,
	int32 index)
{
	return B_ERROR;
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

