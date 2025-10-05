/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef SERVER_MEMORY_ALLOCATOR_H
#define SERVER_MEMORY_ALLOCATOR_H


#include <map>

#include <OS.h>


namespace BPrivate {


struct area_mapping {
	int32	reference_count;
	area_id	server_area;
	area_id local_area;
	uint8*	local_base;
};


class ServerMemoryAllocator {
public:
								ServerMemoryAllocator();
								~ServerMemoryAllocator();

			status_t			InitCheck();

			status_t			AddArea(area_id serverArea, area_id& _localArea,
									uint8*& _base, size_t size,
									bool readOnly = false);
			void				RemoveArea(area_id serverArea);

private:
			std::map<area_id, area_mapping>
								fAreas;
};


}	// namespace BPrivate


#endif	// SERVER_MEMORY_ALLOCATOR_H
