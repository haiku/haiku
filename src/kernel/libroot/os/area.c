/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>
#include "syscalls.h"


area_id
create_area(const char *name, void **start_addr, uint32 addr_spec, size_t size,
	uint32 lock, uint32 protection)
{
	// ToDo: create_area: names don't match, but basic function does.
	// Little work needed on addr_spec.
	return sys_vm_create_anonymous_region(name, start_addr, addr_spec, size, lock, protection);
}


area_id
clone_area(const char *name, void **dest_addr, uint32 addr_spec,
	uint32 protection, area_id source)
{
	// ToDo: Not 100% sure about the REGION_PRIVATE_MAP
	return sys_vm_clone_region(name, dest_addr, addr_spec, source, REGION_PRIVATE_MAP, protection);
}


area_id
find_area(const char *name)
{
	return sys_find_region_by_name(name);
}


area_id
area_for(void *addr)
{
	// ToDo: implement area_for()
	return B_ERROR;
}


status_t
delete_area(area_id id)
{
	return sys_vm_delete_region(id);
}


status_t
resize_area(area_id id, size_t newSize)
{
	// ToDo: implement resize_area()
	return B_ERROR;
}


status_t
set_area_protection(area_id id, uint32 protection)
{
	// ToDo: implement set_area_protection()
	return B_ERROR;
}


status_t
_get_area_info(area_id id, area_info *areaInfo, size_t size)
{
	// size is not yet used, but may, if area_info changes
	(void)size;

	return sys_vm_get_region_info(id, (void *)areaInfo);
}


status_t
_get_next_area_info(team_id team, int32 *cookie, area_info *ainfo, size_t size)
{
	// size is not yet used, but may, if area_info changes
	(void)size;

	// ToDo: implement _get_next_area_info()
	return B_ERROR;
}

