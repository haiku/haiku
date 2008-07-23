/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_VM_STORE_ANONYMOUS_H
#define _KERNEL_VM_STORE_ANONYMOUS_H

#include <vm_types.h>


class VMAnonymousNoSwapCache : public VMCache {
public:
	virtual				~VMAnonymousNoSwapCache();

			status_t	Init(bool canOvercommit, int32 numPrecommittedPages,
							int32 numGuardPages);

	virtual	status_t	Commit(off_t size);
	virtual	bool		HasPage(off_t offset);

	virtual	status_t	Read(off_t offset, const iovec *vecs, size_t count,
							size_t *_numBytes);
	virtual	status_t	Write(off_t offset, const iovec *vecs, size_t count,
							size_t *_numBytes);

	virtual	status_t	Fault(struct vm_address_space *aspace, off_t offset);

	virtual	void		MergeStore(VMCache* source);

private:
	bool	fCanOvercommit;
	bool	fHasPrecommitted;
	uint8	fPrecommittedPages;
	int32	fGuardedSize;
};


#endif	/* _KERNEL_VM_STORE_ANONYMOUS_H */
