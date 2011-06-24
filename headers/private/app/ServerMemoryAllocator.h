/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef SERVER_MEMORY_ALLOCATOR_H
#define SERVER_MEMORY_ALLOCATOR_H


#include <OS.h>
#include <List.h>

namespace BPrivate {

class ServerMemoryAllocator {
	public:
		ServerMemoryAllocator();
		~ServerMemoryAllocator();

		status_t InitCheck();

		status_t AddArea(area_id serverArea, area_id& _localArea, uint8*& _base,
					size_t size, bool readOnly = false);
		void RemoveArea(area_id serverArea);

		status_t AreaAndBaseFor(area_id serverArea, area_id& area, uint8*& base);

	private:
		BList	fAreas;
};

}	// namespace BPrivate

#endif	/* SERVER_MEMORY_ALLOCATOR_H */
