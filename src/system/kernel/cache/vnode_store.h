/*
 * Copyright 2008-2016, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VNODE_STORE_H
#define VNODE_STORE_H


#include <vm/VMCache.h>


struct file_cache_ref;


class VMVnodeCache : public VMCache {
public:
			status_t			Init(struct vnode* vnode,
									uint32 allocationFlags);

	virtual	bool				HasPage(off_t offset);

	virtual	status_t			Read(off_t offset, const generic_io_vec* vecs,
									size_t count, uint32 flags,
									generic_size_t* _numBytes);
	virtual	status_t			Write(off_t offset, const generic_io_vec* vecs,
									size_t count, uint32 flags,
									generic_size_t* _numBytes);
	virtual	status_t			WriteAsync(off_t offset,
									const generic_io_vec* vecs, size_t count,
									generic_size_t numBytes, uint32 flags,
									AsyncIOCallback* callback);
	virtual	bool				CanWritePage(off_t offset);

	virtual	status_t			Fault(struct VMAddressSpace* aspace,
									off_t offset);

	virtual	status_t			AcquireUnreferencedStoreRef();
	virtual	void				AcquireStoreRef();
	virtual	void				ReleaseStoreRef();

	virtual	void				Dump(bool showPages) const;

			void				SetFileCacheRef(file_cache_ref* ref)
									{ fFileCacheRef = ref; }
			file_cache_ref*		FileCacheRef() const
									{ return fFileCacheRef; }

			void				VnodeDeleted()	{ fVnodeDeleted = true; }

			dev_t				DeviceId() const
									{ return fDevice; }
			ino_t				InodeId() const
									{ return fInode; }
			void				SetVnodeID(ino_t id)
									{ fInode = id; }

protected:
	virtual	void				DeleteObject();

private:
			struct vnode*		fVnode;
			file_cache_ref*		fFileCacheRef;
			ino_t				fInode;
			dev_t				fDevice;
	volatile bool				fVnodeDeleted;
};


#endif	/* VNODE_STORE_H */
