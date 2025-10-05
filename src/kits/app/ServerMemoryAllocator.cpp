/*
 * Copyright 2006-2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


/*!	Note, this class don't provide any locking whatsoever - you are
	supposed to have a BPrivate::AppServerLink object around which
	does the necessary locking.
	However, this is not enforced in the methods here, you have to
	take care for yourself!
*/


#include "ServerMemoryAllocator.h"

#include <new>

#ifndef HAIKU_TARGET_PLATFORM_LIBBE_TEST
#	include <syscalls.h>
#endif


static const size_t kReservedSize = 128 * 1024 * 1024;
static const size_t kReserveMaxSize = 32 * 1024 * 1024;


namespace BPrivate {


ServerMemoryAllocator::ServerMemoryAllocator()
{
}


ServerMemoryAllocator::~ServerMemoryAllocator()
{
	while (!fAreas.empty()) {
		std::map<area_id, area_mapping>::iterator it = fAreas.begin();
		area_mapping& mapping = it->second;
		delete_area(mapping.local_area);
		fAreas.erase(it);
	}
}


status_t
ServerMemoryAllocator::InitCheck()
{
	return B_OK;
}


status_t
ServerMemoryAllocator::AddArea(area_id serverArea, area_id& _area,
	uint8*& _base, size_t size, bool readOnly)
{
	std::map<area_id, area_mapping>::iterator it = fAreas.find(serverArea);
	if (it != fAreas.end()) {
		area_mapping& mapping = it->second;
		mapping.reference_count++;

		_area = mapping.local_area;
		_base = mapping.local_base;

		return B_OK;
	}

	area_mapping* mapping;
	try {
		mapping = &fAreas[serverArea];
	} catch (const std::bad_alloc&) {
		return B_NO_MEMORY;
	}
	mapping->reference_count = 1;

	status_t status = B_ERROR;
	uint32 addressSpec = B_ANY_ADDRESS;
	void* base;
#ifndef HAIKU_TARGET_PLATFORM_LIBBE_TEST
	if (!readOnly && size < kReserveMaxSize) {
		// Reserve 128 MB of space for the area, but only if the area
		// is smaller than 32 MB (else the address space waste would
		// likely to be too large)
		base = (void*)0x60000000;
		status = _kern_reserve_address_range((addr_t*)&base, B_BASE_ADDRESS,
			kReservedSize);
		addressSpec = status == B_OK ? B_EXACT_ADDRESS : B_BASE_ADDRESS;
	}
#endif

	mapping->local_area = clone_area(readOnly
			? "server read-only memory" : "server_memory", &base, addressSpec,
		B_CLONEABLE_AREA | B_READ_AREA | (readOnly ? 0 : B_WRITE_AREA),
		serverArea);
	if (mapping->local_area < B_OK) {
		status = mapping->local_area;

		fAreas.erase(serverArea);

		return status;
	}

	mapping->server_area = serverArea;
	mapping->local_base = (uint8*)base;

	_area = mapping->local_area;
	_base = mapping->local_base;

	return B_OK;
}


void
ServerMemoryAllocator::RemoveArea(area_id serverArea)
{
	std::map<area_id, area_mapping>::iterator it = fAreas.find(serverArea);
	if (it != fAreas.end()) {
		area_mapping& mapping = it->second;
		if (mapping.reference_count-- == 1) {
			delete_area(mapping.local_area);
			fAreas.erase(serverArea);
		}
	}
}


}	// namespace BPrivate
