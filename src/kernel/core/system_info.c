/*
** Copyright 2004, Stefano Ceccherini. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>
#include <KernelExport.h>

#include <system_info.h>
#include <vm.h>
#include <debug.h>
#include <port.h>
#include <real_time_clock.h>
#include <sem.h>
#include <smp.h>

#include <string.h>


const static int64 kKernelVersion = 0x1;
const static char kKernelName[] = "kernel_" OBOS_ARCH;


status_t
_get_system_info(system_info *info, size_t size)
{
	if (size != sizeof(system_info))
		return B_BAD_VALUE;

	memset(info, 0, sizeof(system_info));
	// TODO: Add:
	// - max_pages
	// - used_pages
	// - page_faults
	// - max_threads
	// - used_threads
	// - max_teams
	// - used_teams

	info->boot_time = rtc_boot_time();
	info->cpu_count = smp_get_num_cpus();
	info->used_ports = port_used_ports();
	info->max_ports = port_max_ports();
	info->used_sems = sem_used_sems();
	info->max_sems = sem_max_sems();

	info->kernel_version = kKernelVersion;
	strlcpy(info->kernel_name, kKernelName, B_FILE_NAME_LENGTH);
	strlcpy(info->kernel_build_date, __DATE__, B_OS_NAME_LENGTH);
	strlcpy(info->kernel_build_time, __TIME__, B_OS_NAME_LENGTH);

	// TODO: Add arch specific stuff (arch_get_system_info() ?)
	// - cpu_type
	// - cpu_revision
	// - various cpu_info
	// - cpu_clock_speed
	// - bus_clock_speed
	// - platform_type

	return B_OK;
}


status_t
_user_get_system_info(system_info *userInfo, size_t size)
{
	system_info info;
	status_t status;

	// The BeBook says get_system_info() always returns B_OK,
	// but that ain't true with invalid addresses
	if (userInfo == NULL || !IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;

	status = _get_system_info(&info, size);
	if (status == B_OK) {
		if (user_memcpy(userInfo, &info, sizeof(system_info)) < B_OK)
			return B_BAD_ADDRESS;

		return B_OK;
	}

	return status;
}
