/*
 * Copyright 2008-2016, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VNODE_STORE_H
#define VNODE_STORE_H


#include <vm/VMCache.h>
#include <Referenceable.h>


struct file_cache_ref;


class VMVnodeCache final : public VMCache {
public:
			status_t			Init(struct vnode* vnode,
									ModifiedPageQueue* modifiedQueue,
									uint32 allocationFlags);
								VMVnodeCache();
	virtual						~VMVnodeCache();

	virtual	status_t			Commit(off_t size, int priority);
	virtual	bool				StoreHasPage(off_t offset);

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
	virtual	ModifiedPageQueue*	ModifiedQueue();

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

protected:
	virtual	void				DeleteObject();

private:
			struct vnode*		fVnode;
			file_cache_ref*		fFileCacheRef;
			ino_t				fInode;
			dev_t				fDevice;
	BReference<ModifiedPageQueue> fModifiedPageQueue;
	volatile bool				fVnodeDeleted;
};


#endif	/* VNODE_STORE_H */
