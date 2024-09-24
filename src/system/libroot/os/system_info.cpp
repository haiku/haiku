/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>

#include <string.h>

#include <algorithm>

#include <syscalls.h>
#include <system_info.h>


#if __HAIKU_BEOS_COMPATIBLE


#define LEGACY_B_CPU_X86			15

#define LEGACY_B_AT_CLONE_PLATFORM	2

typedef struct {
	bigtime_t	active_time;	/* usec of doing useful work since boot */
} legacy_cpu_info;

typedef struct {
	int32			id[2];				/* unique machine ID */
	bigtime_t		boot_time;			/* time of boot (usecs since 1/1/1970) */

	int32			cpu_count;			/* number of cpus */
	int32			cpu_type;			/* type of cpu */
	int32			cpu_revision;		/* revision # of cpu */
	legacy_cpu_info	cpu_infos[8];		/* info about individual cpus */
	int64			cpu_clock_speed;	/* processor clock speed (Hz) */
	int64			bus_clock_speed;	/* bus clock speed (Hz) */
	int32			platform_type;	/* type of machine we're on */

	int32			max_pages;			/* total # of accessible pages */
	int32			used_pages;			/* # of accessible pages in use */
	int32			page_faults;		/* # of page faults */
	int32			max_sems;
	int32			used_sems;
	int32			max_ports;
	int32			used_ports;
	int32			max_threads;
	int32			used_threads;
	int32			max_teams;
	int32			used_teams;

	char			kernel_name[256];
	char			kernel_build_date[32];
	char			kernel_build_time[32];
	int64			kernel_version;

	bigtime_t		_busy_wait_time;	/* reserved for whatever */

	int32			cached_pages;
	uint32			abi;				/* the system API */
	int32			ignored_pages;		/* # of ignored/inaccessible pages */
	int32			pad;
} legacy_system_info;


extern "C" status_t
_get_system_info(legacy_system_info* info, size_t size)
{
	if (info == NULL || size != sizeof(legacy_system_info))
		return B_BAD_VALUE;
	memset(info, 0, sizeof(legacy_system_info));

	system_info systemInfo;
	status_t error = _kern_get_system_info(&systemInfo);
	if (error != B_OK)
		return error;

	cpu_info cpuInfos[8];
	error = _kern_get_cpu_info(0, std::min(systemInfo.cpu_count, uint32(8)),
			cpuInfos);
	if (error != B_OK)
		return error;

	info->boot_time = systemInfo.boot_time;
	info->cpu_count = std::min(systemInfo.cpu_count, uint32(8));
	for (int32 i = 0; i < info->cpu_count; i++)
		info->cpu_infos[i].active_time = cpuInfos[i].active_time;

	info->platform_type = LEGACY_B_AT_CLONE_PLATFORM;
	info->cpu_type = LEGACY_B_CPU_X86;

	uint32 topologyNodeCount = 0;
	cpu_topology_node_info* topology = NULL;
	error = get_cpu_topology_info(NULL, &topologyNodeCount);
	if (error != B_OK)
		return B_OK;
	if (topologyNodeCount != 0) {
		topology = new(std::nothrow) cpu_topology_node_info[topologyNodeCount];
		if (topology == NULL)
			return B_NO_MEMORY;
	}
	error = get_cpu_topology_info(topology, &topologyNodeCount);
	if (error != B_OK) {
		delete[] topology;
		return error;
	}

	for (uint32 i = 0; i < topologyNodeCount; i++) {
		if (topology[i].type == B_TOPOLOGY_CORE) {
			info->cpu_clock_speed = topology[i].data.core.default_frequency;
			break;
		}
	}
	info->bus_clock_speed = info->cpu_clock_speed;
	delete[] topology;

	info->max_pages = std::min(systemInfo.max_pages, uint64(INT32_MAX));
	info->used_pages = std::min(systemInfo.used_pages, uint64(INT32_MAX));
	info->cached_pages = std::min(systemInfo.cached_pages, uint64(INT32_MAX));
	info->ignored_pages = std::min(systemInfo.ignored_pages, uint64(INT32_MAX));
	info->page_faults = std::min(systemInfo.page_faults, uint32(INT32_MAX));
	info->max_sems = std::min(systemInfo.max_sems, uint32(INT32_MAX));
	info->used_sems = std::min(systemInfo.used_sems, uint32(INT32_MAX));
	info->max_ports = std::min(systemInfo.max_ports, uint32(INT32_MAX));
	info->used_ports = std::min(systemInfo.used_ports, uint32(INT32_MAX));
	info->max_threads = std::min(systemInfo.max_threads, uint32(INT32_MAX));
	info->used_threads = std::min(systemInfo.used_threads, uint32(INT32_MAX));
	info->max_teams = std::min(systemInfo.max_teams, uint32(INT32_MAX));
	info->used_teams = std::min(systemInfo.used_teams, uint32(INT32_MAX));

	strlcpy(info->kernel_name, systemInfo.kernel_name,
		sizeof(info->kernel_name));
	strlcpy(info->kernel_build_date, systemInfo.kernel_build_date,
		sizeof(info->kernel_build_date));
	strlcpy(info->kernel_build_time, systemInfo.kernel_build_time,
		sizeof(info->kernel_build_time));
	info->kernel_version = systemInfo.kernel_version;

	info->abi = systemInfo.abi;

	return B_OK;
}


#endif	// __HAIKU_BEOS_COMPATIBLE


status_t
__get_system_info(system_info* info)
{
	return _kern_get_system_info(info);
}


typedef struct {
	bigtime_t	active_time;
	bool		enabled;
} beta2_cpu_info;


extern "C" status_t
__get_cpu_info(uint32 firstCPU, uint32 cpuCount, beta2_cpu_info* beta2_info)
{
	cpu_info info;
	status_t err = _get_cpu_info_etc(firstCPU, cpuCount, &info, sizeof(info));
	if (err == B_OK) {
		beta2_info->active_time = info.active_time;
		beta2_info->enabled = info.enabled;
	}
	return err;
}


status_t
_get_cpu_info_etc(uint32 firstCPU, uint32 cpuCount, cpu_info* info,
	size_t size)
{
	if (info == NULL || size != sizeof(cpu_info))
		return B_BAD_VALUE;
	return _kern_get_cpu_info(firstCPU, cpuCount, info);
}


status_t
__get_cpu_topology_info(cpu_topology_node_info* topologyInfos,
	uint32* topologyInfoCount)
{
	return _kern_get_cpu_topology_info(topologyInfos, topologyInfoCount);
}


status_t
__start_watching_system(int32 object, uint32 flags, port_id port, int32 token)
{
	return _kern_start_watching_system(object, flags, port, token);
}


status_t
__stop_watching_system(int32 object, uint32 flags, port_id port, int32 token)
{
	return _kern_stop_watching_system(object, flags, port, token);
}


int32
is_computer_on(void)
{
	return _kern_is_computer_on();
}


double
is_computer_on_fire(void)
{
	return 0.63739;
}


B_DEFINE_WEAK_ALIAS(__get_system_info, get_system_info);
B_DEFINE_WEAK_ALIAS(__get_cpu_info, get_cpu_info);
B_DEFINE_WEAK_ALIAS(__get_cpu_topology_info, get_cpu_topology_info);

