/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002-2004, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include "io_resources.h"

#include <stdlib.h>
#include <string.h>


//#define TRACE_IO_RESOURCES
#ifdef TRACE_IO_RESOURCES
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


typedef DoublyLinkedList<io_resource_private,
	DoublyLinkedListMemberGetLink<io_resource_private,
		&io_resource_private::fTypeLink> > ResourceTypeList;


static ResourceTypeList sMemoryList;
static ResourceTypeList sPortList;
static ResourceTypeList sDMAChannelList;


io_resource_private::io_resource_private()
{
	_Init();
}


io_resource_private::~io_resource_private()
{
	Release();
}


void
io_resource_private::_Init()
{
	type = 0;
	base = 0;
	length = 0;
}


status_t
io_resource_private::Acquire(const io_resource& resource)
{
	if (!_IsValid(resource))
		return B_BAD_VALUE;

	type = resource.type;
	base = resource.base;

	if (type != B_ISA_DMA_CHANNEL)
		length = resource.length;
	else
		length = 1;

	ResourceTypeList* list = NULL;

	switch (type) {
		case B_IO_MEMORY:
			list = &sMemoryList;
			break;
		case B_IO_PORT:
			list = &sPortList;
			break;
		case B_ISA_DMA_CHANNEL:
			list = &sDMAChannelList;
			break;
	}

	ResourceTypeList::Iterator iterator = list->GetIterator();
	while (iterator.HasNext()) {
		io_resource* resource = iterator.Next();

		// we need the "base + length - 1" trick to avoid wrap around at 4 GB
		if (resource->base >= base
			&& resource->base + length - 1 <= base + length - 1) {
			// This range is already covered by someone else
			// TODO: we might want to ignore resources that belong to
			// a node that isn't used.
			_Init();
			return B_RESOURCE_UNAVAILABLE;
		}
	}

	list->Add(this);
	return B_OK;
}


void
io_resource_private::Release()
{
	if (type == 0)
		return;

	switch (type) {
		case B_IO_MEMORY:
			sMemoryList.Remove(this);
			break;
		case B_IO_PORT:
			sPortList.Remove(this);
			break;
		case B_ISA_DMA_CHANNEL:
			sDMAChannelList.Remove(this);
			break;
	}

	_Init();
}


/*static*/ bool
io_resource_private::_IsValid(const io_resource& resource)
{
	switch (resource.type) {
		case B_IO_MEMORY:
			return resource.base + resource.length > resource.base;
		case B_IO_PORT:
			return (uint16)resource.base == resource.base
				&& (uint16)resource.length == resource.length
				&& resource.base + resource.length > resource.base;
		case B_ISA_DMA_CHANNEL:
			return resource.base <= 8;

		default:
			return false;
	}
}


//	#pragma mark -


void
dm_init_io_resources(void)
{
	new(&sMemoryList) ResourceTypeList;
	new(&sPortList) ResourceTypeList;
	new(&sDMAChannelList) ResourceTypeList;
}
