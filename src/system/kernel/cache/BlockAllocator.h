/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef BLOCK_ALLOCATOR_H
#define BLOCK_ALLOCATOR_H


#include <util/DoublyLinkedList.h>
#include <lock.h>


class BlockAllocator : public DoublyLinkedListLinkImpl<BlockAllocator> {
	public:
		void *Get();
		void Put(void *block);

		static BlockAllocator *GetAllocator(size_t size);
		static void PutAllocator(BlockAllocator *allocator);

	protected:
		BlockAllocator(size_t size);
		~BlockAllocator();

		size_t Size() const { return fSize; }

		void Acquire();
		int32 Release();

	private:
		typedef DoublyLinkedList<BlockAllocator> AllocatorList;

		size_t	fSize;
		int32	fRefCount;

	public:
		static AllocatorList sList;
		static mutex sMutex;
};

extern "C" status_t init_block_allocator(void);

#endif	/* BLOCK_ALLOCATOR_H */
