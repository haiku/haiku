/*
 * Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>

#include <libroot_private.h>

#include "syscalls.h"


area_id
create_area(const char *name, void **address, uint32 addressSpec, size_t size,
	uint32 lock, uint32 protection)
{
	if (__gABIVersion < B_HAIKU_ABI_GCC_2_HAIKU)
		protection |= B_EXECUTE_AREA;
	if (__gAPIVersion < B_HAIKU_VERSION_1_PRE_BETA_2)
		protection |= B_CLONEABLE_AREA;
	return _kern_create_area(name, address, addressSpec, size, lock, protection);
}


area_id
clone_area(const char *name, void **address, uint32 addressSpec,
	uint32 protection, area_id sourceArea)
{
	if (__gABIVersion < B_HAIKU_ABI_GCC_2_HAIKU)
		protection |= B_EXECUTE_AREA | B_CLONEABLE_AREA;
	return _kern_clone_area(name, address, addressSpec, protection, sourceArea);
}


area_id
find_area(const char *name)
{
	return _kern_find_area(name);
}


area_id
area_for(void *address)
{
	return _kern_area_for(address);
}


status_t
delete_area(area_id id)
{
	return _kern_delete_area(id);
}


status_t
resize_area(area_id id, size_t newSize)
{
	return _kern_resize_area(id, newSize);
}


status_t
set_area_protection(area_id id, uint32 protection)
{
	if (__gABIVersion < B_HAIKU_ABI_GCC_2_HAIKU)
		protection |= B_EXECUTE_AREA;
	return _kern_set_area_protection(id, protection);
}


status_t
_get_area_info(area_id id, area_info *areaInfo, size_t size)
{
	// size is not yet used, but may, if area_info changes
	(void)size;

	return _kern_get_area_info(id, areaInfo);
}


status_t
_get_next_area_info(team_id team, ssize_t *cookie, area_info *areaInfo, size_t size)
{
	// size is not yet used, but may, if area_info changes
	(void)size;

	return _kern_get_next_area_info(team, cookie, areaInfo);
}

