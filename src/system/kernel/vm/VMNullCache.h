/*
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2005-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_VM_STORE_NULL_H
#define _KERNEL_VM_STORE_NULL_H


#include <vm/VMCache.h>


class VMNullCache : public VMCache {
public:
			status_t			Init(uint32 allocationFlags);

protected:
	virtual	void				DeleteObject();
};


#endif	/* _KERNEL_VM_STORE_NULL_H */
