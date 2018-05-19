/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_COMPAT_OS_H
#define _KERNEL_COMPAT_OS_H


typedef struct {
	bigtime_t		boot_time;			/* time of boot (usecs since 1/1/1970) */

	uint32			cpu_count;			/* number of cpus */

	uint64			max_pages;			/* total # of accessible pages */
	uint64			used_pages;			/* # of accessible pages in use */
	uint64			cached_pages;
	uint64			block_cache_pages;
	uint64			ignored_pages;		/* # of ignored/inaccessible pages */

	uint64			needed_memory;
	uint64			free_memory;

	uint64			max_swap_pages;
	uint64			free_swap_pages;

	uint32			page_faults;		/* # of page faults */

	uint32			max_sems;
	uint32			used_sems;

	uint32			max_ports;
	uint32			used_ports;

	uint32			max_threads;
	uint32			used_threads;

	uint32			max_teams;
	uint32			used_teams;

	char			kernel_name[B_FILE_NAME_LENGTH];
	char			kernel_build_date[B_OS_NAME_LENGTH];
	char			kernel_build_time[B_OS_NAME_LENGTH];

	int64			kernel_version;
	uint32			abi;				/* the system API */
} _PACKED compat_system_info;


static_assert(sizeof(compat_system_info) == 0x1c4,
	"size of compat_system_info mismatch");


inline status_t
copy_ref_var_to_user(system_info &info, system_info* userInfo)
{
	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		compat_system_info compat_info;
		compat_info.boot_time = info.boot_time;
		compat_info.cpu_count = info.cpu_count;
		compat_info.max_pages = info.max_pages;
		compat_info.used_pages = info.used_pages;
		compat_info.cached_pages = info.cached_pages;
		compat_info.block_cache_pages = info.block_cache_pages;
		compat_info.ignored_pages = info.ignored_pages;
		compat_info.needed_memory = info.needed_memory;
		compat_info.free_memory = info.free_memory;
		compat_info.needed_memory = info.needed_memory;
		compat_info.max_swap_pages = info.max_swap_pages;
		compat_info.free_swap_pages = info.free_swap_pages;
		compat_info.max_sems = info.max_sems;
		compat_info.used_sems = info.used_sems;
		compat_info.max_ports = info.max_ports;
		compat_info.used_ports = info.used_ports;
		compat_info.max_threads = info.max_threads;
		compat_info.used_threads = info.used_threads;
		compat_info.max_teams = info.max_teams;
		compat_info.used_teams = info.used_teams;
		strlcpy(compat_info.kernel_name, info.kernel_name, B_FILE_NAME_LENGTH);
		strlcpy(compat_info.kernel_build_date, info.kernel_build_date,
			B_OS_NAME_LENGTH);
		strlcpy(compat_info.kernel_build_time, info.kernel_build_time,
			B_OS_NAME_LENGTH);
		compat_info.kernel_version = info.kernel_version;
		compat_info.abi = info.abi;
		if (user_memcpy(userInfo, &compat_info, sizeof(compat_info)) < B_OK)
			return B_BAD_ADDRESS;
	} else if (user_memcpy(userInfo, &info, sizeof(system_info)) < B_OK)
		return B_BAD_ADDRESS;
	return B_OK;
}


#endif // _KERNEL_COMPAT_OS_H
