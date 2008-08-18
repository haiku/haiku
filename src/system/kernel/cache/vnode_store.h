/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VNODE_STORE_H
#define VNODE_STORE_H


#include <vm_types.h>


struct file_cache_ref;


class VMVnodeCache : public VMCache {
public:
			status_t	Init(struct vnode *vnode);

	virtual	bool		HasPage(off_t offset);

	virtual	status_t	Read(off_t offset, const iovec *vecs, size_t count,
							size_t *_numBytes);
	virtual	status_t	Write(off_t offset, const iovec *vecs, size_t count,
							uint32 flags, size_t *_numBytes);
	virtual	status_t	WriteAsync(off_t offset, const iovec* vecs,
							size_t count, size_t numBytes, uint32 flags,
							AsyncIOCallback* callback);

	virtual	status_t	Fault(struct vm_address_space *aspace, off_t offset);

	virtual	status_t	AcquireUnreferencedStoreRef();
	virtual	void		AcquireStoreRef();
	virtual	void		ReleaseStoreRef();

			void		SetFileCacheRef(file_cache_ref* ref)
							{ fFileCacheRef = ref; }
	file_cache_ref*		FileCacheRef() const
							{ return fFileCacheRef; }

private:
	struct vnode*	fVnode;
	dev_t			fDevice;
	ino_t			fInode;
	file_cache_ref*	fFileCacheRef;
};


#endif	/* VNODE_STORE_H */
