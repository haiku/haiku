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


#define compat_size_t uint32
#define compat_ptr_t uint32
typedef struct compat_area_info {
	area_id		area;
	char		name[B_OS_NAME_LENGTH];
	compat_size_t	size;
	uint32		lock;
	uint32		protection;
	team_id		team;
	uint32		ram_size;
	uint32		copy_count;
	uint32		in_count;
	uint32		out_count;
	compat_ptr_t	address;
} _PACKED compat_area_info;


static_assert(sizeof(compat_area_info) == 0x48,
	"size of compat_area_info mismatch");


inline status_t
copy_ref_var_to_user(area_info &info, area_info* userInfo)
{
	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		compat_area_info compatInfo;
		compatInfo.area = info.area;
		strlcpy(compatInfo.name, info.name, B_OS_NAME_LENGTH);
		compatInfo.size = info.size;
		compatInfo.lock = info.lock;
		compatInfo.protection = info.protection;
		compatInfo.team = info.team;
		compatInfo.ram_size = info.ram_size;
		compatInfo.copy_count = info.copy_count;
		compatInfo.in_count = info.in_count;
		compatInfo.out_count = info.out_count;
		compatInfo.address = (compat_ptr_t)(addr_t)info.address;
		if (user_memcpy(userInfo, &compatInfo, sizeof(compatInfo)) < B_OK)
			return B_BAD_ADDRESS;
	} else if (user_memcpy(userInfo, &info, sizeof(info)) < B_OK) {
		return B_BAD_ADDRESS;
	}
	return B_OK;
}


typedef struct {
	thread_id		thread;
	team_id			team;
	char			name[B_OS_NAME_LENGTH];
	thread_state	state;
	int32			priority;
	sem_id			sem;
	bigtime_t		user_time;
	bigtime_t		kernel_time;
	uint32			stack_base;
	uint32			stack_end;
} _PACKED compat_thread_info;


static_assert(sizeof(compat_thread_info) == 76,
	"size of compat_thread_info mismatch");


inline status_t
copy_ref_var_to_user(thread_info &info, thread_info* userInfo)
{
	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		compat_thread_info compatInfo;
		compatInfo.thread = info.thread;
		compatInfo.team = info.team;
		strlcpy(compatInfo.name, info.name, sizeof(compatInfo.name));
		compatInfo.state = info.state;
		compatInfo.priority = info.priority;
		compatInfo.sem = info.sem;
		compatInfo.user_time = info.user_time;
		compatInfo.kernel_time = info.kernel_time;
		compatInfo.stack_base = (uint32)(addr_t)info.stack_base;
		compatInfo.stack_end = (uint32)(addr_t)info.stack_end;
		if (user_memcpy(userInfo, &compatInfo, sizeof(compatInfo)) < B_OK)
			return B_BAD_ADDRESS;
	} else if (user_memcpy(userInfo, &info, sizeof(info)) < B_OK) {
		return B_BAD_ADDRESS;
	}
	return B_OK;
}


inline status_t
copy_ref_var_to_user(void* &addr, void** userAddr)
{
	if (!IS_USER_ADDRESS(userAddr))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		compat_ptr_t compatAddr = (uint32)(addr_t)addr;
		if (user_memcpy(userAddr, &compatAddr, sizeof(compatAddr)) < B_OK)
			return B_BAD_ADDRESS;
	} else if (user_memcpy(userAddr, &addr, sizeof(addr)) < B_OK) {
		return B_BAD_ADDRESS;
	}
	return B_OK;
}


inline status_t
copy_ref_var_from_user(void** userAddr, void* &addr)
{
	if (!IS_USER_ADDRESS(userAddr))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		compat_ptr_t compatAddr;
		if (user_memcpy(&compatAddr, userAddr, sizeof(compatAddr)) < B_OK)
			return B_BAD_ADDRESS;
		addr = (void*)(addr_t)compatAddr;
	} else if (user_memcpy(&addr, userAddr, sizeof(addr)) < B_OK) {
		return B_BAD_ADDRESS;
	}
	return B_OK;
}


inline status_t
copy_ref_var_to_user(addr_t &addr, addr_t* userAddr)
{
	if (!IS_USER_ADDRESS(userAddr))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		uint32 compatAddr = (uint32)addr;
		if (user_memcpy(userAddr, &compatAddr, sizeof(compatAddr)) < B_OK)
			return B_BAD_ADDRESS;
	} else if (user_memcpy(userAddr, &addr, sizeof(addr)) < B_OK) {
		return B_BAD_ADDRESS;
	}
	return B_OK;
}


inline status_t
copy_ref_var_from_user(addr_t* userAddr, addr_t &addr)
{
	if (!IS_USER_ADDRESS(userAddr))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		uint32 compatAddr;
		if (user_memcpy(&compatAddr, userAddr, sizeof(compatAddr)) < B_OK)
			return B_BAD_ADDRESS;
		addr = (addr_t)compatAddr;
	} else if (user_memcpy(&addr, userAddr, sizeof(addr)) < B_OK) {
		return B_BAD_ADDRESS;
	}
	return B_OK;
}


inline status_t
copy_ref_var_to_user(ssize_t &size, ssize_t* userSize)
{
	if (!IS_USER_ADDRESS(userSize))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		int32 compatSize = (int32)size;
		if (user_memcpy(userSize, &compatSize, sizeof(compatSize)) < B_OK)
			return B_BAD_ADDRESS;
	} else if (user_memcpy(userSize, &size, sizeof(size)) < B_OK) {
		return B_BAD_ADDRESS;
	}
	return B_OK;
}


inline status_t
copy_ref_var_from_user(ssize_t* userSize, ssize_t &size)
{
	if (!IS_USER_ADDRESS(userSize))
		return B_BAD_ADDRESS;
	Thread* thread = thread_get_current_thread();
	bool compatMode = (thread->flags & THREAD_FLAGS_COMPAT_MODE) != 0;
	if (compatMode) {
		int32 compatSize;
		if (user_memcpy(&compatSize, userSize, sizeof(compatSize)) < B_OK)
			return B_BAD_ADDRESS;
		size = (ssize_t)compatSize;
	} else if (user_memcpy(&size, userSize, sizeof(size)) < B_OK) {
		return B_BAD_ADDRESS;
	}
	return B_OK;
}


#endif // _KERNEL_COMPAT_OS_H
