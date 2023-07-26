/*
 * Copyright 2023, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_SELECT_SYNC_H
#define _KERNEL_SELECT_SYNC_H

#include <wait_for_objects.h>
#include <Referenceable.h>


struct select_sync : public BReferenceable {
	virtual ~select_sync();

	virtual status_t Notify(select_info* info, uint16 events) = 0;
};


#endif	// _KERNEL_SELECT_SYNC_H
