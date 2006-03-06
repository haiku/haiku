/*
 * Copyright (c) 2004-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stefano Ceccherini
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <OS.h>
#include <KernelExport.h>

#include <system_info.h>
#include <arch/system_info.h>

#include <cpu.h>
#include <debug.h>
#include <port.h>
#include <real_time_clock.h>
#include <sem.h>
#include <smp.h>
#include <team.h>
#include <thread.h>
#include <vm.h>
#include <vm_page.h>

#include <string.h>


const static int64 kKernelVersion = 0x1;
const static char *kKernelName = "kernel_" HAIKU_ARCH;


static int
dump_info(int argc, char **argv)
{
	int32 i;

	kprintf("kernel build: %s %s\n\n", __DATE__, __TIME__);
	kprintf("cpu count: %ld, active times:\n", smp_get_num_cpus());

	for (i = 0; i < smp_get_num_cpus(); i++)
		kprintf("  [%ld] %Ld\n", i + 1, cpu_get_active_time(i));

	// ToDo: Add page_faults
	kprintf("pages:\t\t%ld (%ld max)\n", vm_page_num_pages() - vm_page_num_free_pages(),
		vm_page_num_pages());

	kprintf("sems:\t\t%ld (%ld max)\n", sem_used_sems(), sem_max_sems());
	kprintf("ports:\t\t%ld (%ld max)\n", port_used_ports(), port_max_ports());
	kprintf("threads:\t%ld (%ld max)\n", thread_used_threads(), thread_max_threads());
	kprintf("teams:\t\t%ld (%ld max)\n", team_used_teams(), team_max_teams());

	return 0;
}


//	#pragma mark -


status_t
_get_system_info(system_info *info, size_t size)
{
	int32 i;

	if (size != sizeof(system_info))
		return B_BAD_VALUE;

	memset(info, 0, sizeof(system_info));

	info->boot_time = rtc_boot_time();
	info->cpu_count = smp_get_num_cpus();

	for (i = 0; i < info->cpu_count; i++)
		info->cpu_infos[i].active_time = cpu_get_active_time(i);

	// ToDo: Add page_faults
	info->max_pages = vm_page_num_pages();
	info->used_pages = vm_page_num_pages() - vm_page_num_free_pages();

	info->used_threads = thread_used_threads();
	info->max_threads = thread_max_threads();
	info->used_teams = team_used_teams();
	info->max_teams = team_max_teams();
	info->used_ports = port_used_ports();
	info->max_ports = port_max_ports();
	info->used_sems = sem_used_sems();
	info->max_sems = sem_max_sems();

	info->kernel_version = kKernelVersion;
	strlcpy(info->kernel_name, kKernelName, B_FILE_NAME_LENGTH);
	strlcpy(info->kernel_build_date, __DATE__, B_OS_NAME_LENGTH);
	strlcpy(info->kernel_build_time, __TIME__, B_OS_NAME_LENGTH);

	// all other stuff is architecture specific
	return arch_get_system_info(info, size);
}


status_t 
system_info_init(struct kernel_args *args)
{
	add_debugger_command("info", &dump_info, "System info");

	return arch_system_info_init(args);
}


//	#pragma mark -


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
