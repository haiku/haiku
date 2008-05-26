/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002-04, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef IO_RESOURCES_H
#define IO_RESOURCES_H


#include <device_manager.h>

#include <util/DoublyLinkedList.h>


struct io_resource_private : io_resource,
		DoublyLinkedListLinkImpl<io_resource_private> {
						io_resource_private();
						~io_resource_private();

			status_t	Acquire(const io_resource& resource);
			void		Release();

private:
			void		_Init();
	static	bool		_IsValid(const io_resource& resource);

public:
	DoublyLinkedListLink<io_resource_private> fTypeLink;
};

typedef DoublyLinkedList<io_resource_private> ResourceList;


#ifdef __cplusplus
extern "C" {
#endif

void dm_init_io_resources(void);

#ifdef __cplusplus
}
#endif

#endif	/* IO_RESOURCES_H */
