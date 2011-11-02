/*
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2005-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_VM_STORE_DEVICE_H
#define _KERNEL_VM_STORE_DEVICE_H


#include <vm/VMCache.h>


class VMDeviceCache : public VMCache {
public:
			status_t			Init(addr_t baseAddress,
									uint32 allocationFlags);

	virtual	status_t			Read(off_t offset, const iovec* vecs,
									size_t count, uint32 flags,
						 			size_t* _numBytes);
	virtual	status_t			Write(off_t offset, const iovec* vecs,
									size_t count, uint32 flags,
						  			size_t* _numBytes);

protected:
	virtual	void				DeleteObject();

private:
			addr_t				fBaseAddress;
};


#endif	/* _KERNEL_VM_STORE_DEVICE_H */
