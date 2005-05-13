/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "BlockAllocator.h"

#include <KernelExport.h>
#include <util/AutoLock.h>


BlockAllocator::AllocatorList BlockAllocator::sList;
mutex BlockAllocator::sMutex;


BlockAllocator::BlockAllocator(size_t size)
	:
	fSize(size)
{
}


BlockAllocator::~BlockAllocator()
{
}


void *
BlockAllocator::Get()
{
	return malloc(fSize);
}


void
BlockAllocator::Put(void *block)
{
	free(block);
}


void
BlockAllocator::Acquire()
{
	fRefCount++;
}


int32
BlockAllocator::Release()
{
	return --fRefCount;
}


//	#pragma mark - static


// static
BlockAllocator *
BlockAllocator::GetAllocator(size_t size)
{
	MutexLocker locker(&sMutex);

	// search for allocator in list

	AllocatorList::Iterator iterator = sList.GetIterator();
	while (iterator.HasNext()) {
		BlockAllocator *allocator = iterator.Next();

		if (allocator->Size() == size) {
			allocator->Acquire();
			return allocator;
		}
	}

	// it's not yet there, create new one

	BlockAllocator *allocator = new BlockAllocator(size);
	if (allocator == NULL)
		return NULL;

	sList.Add(allocator);
	return allocator;
}


// static
void
BlockAllocator::PutAllocator(BlockAllocator *allocator)
{
	MutexLocker locker(&sMutex);

	if (!allocator->Release()) {
		sList.Remove(allocator);
		delete allocator;
	}
}


//	#pragma mark -


extern "C" status_t
init_block_allocator(void)
{
	status_t status = mutex_init(&BlockAllocator::sMutex, "block allocator");
	if (status < B_OK)
		return status;

	new(&BlockAllocator::sList) DoublyLinkedList<BlockAllocator>;
		// static initializers do not work in the kernel,
		// so we have to do it here, manually
	return B_OK;
}

