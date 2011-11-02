/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_VM_STORE_ANONYMOUS_H
#define _KERNEL_VM_STORE_ANONYMOUS_H


#include <vm/VMCache.h>


#if ENABLE_SWAP_SUPPORT

typedef uint32 swap_addr_t;
	// TODO: Should be wider, but RadixBitmap supports only a 32 bit type ATM!
struct swap_block;
struct system_memory_info;


extern "C" {
	void swap_init(void);
	void swap_init_post_modules(void);
	bool swap_free_page_swap_space(vm_page* page);
	uint32 swap_available_pages(void);
	uint32 swap_total_swap_pages(void);
}


class VMAnonymousCache : public VMCache {
public:
	virtual						~VMAnonymousCache();

			status_t			Init(bool canOvercommit,
									int32 numPrecommittedPages,
									int32 numGuardPages,
									uint32 allocationFlags);

	virtual	status_t			Resize(off_t newSize, int priority);

	virtual	status_t			Commit(off_t size, int priority);
	virtual	bool				HasPage(off_t offset);
	virtual	bool				DebugHasPage(off_t offset);

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

	virtual	int32				MaxPagesPerAsyncWrite() const;

	virtual	status_t			Fault(struct VMAddressSpace* aspace,
									off_t offset);

	virtual	void				Merge(VMCache* source);

protected:
	virtual	void				DeleteObject();

private:
			class WriteCallback;
			friend class WriteCallback;

			void				_SwapBlockBuild(off_t pageIndex,
									swap_addr_t slotIndex, uint32 count);
			void        		_SwapBlockFree(off_t pageIndex, uint32 count);
			swap_addr_t			_SwapBlockGetAddress(off_t pageIndex);
			status_t			_Commit(off_t size, int priority);

			void				_MergePagesSmallerSource(
									VMAnonymousCache* source);
			void				_MergePagesSmallerConsumer(
									VMAnonymousCache* source);
			void				_MergeSwapPages(VMAnonymousCache* source);

private:
	friend bool swap_free_page_swap_space(vm_page* page);

			bool				fCanOvercommit;
			bool				fHasPrecommitted;
			uint8				fPrecommittedPages;
			int32				fGuardedSize;
			off_t   			fCommittedSwapSize;
			off_t   			fAllocatedSwapSize;
};

#endif	// ENABLE_SWAP_SUPPORT


extern "C" void swap_get_info(struct system_memory_info* info);


#endif	/* _KERNEL_VM_STORE_ANONYMOUS_H */
