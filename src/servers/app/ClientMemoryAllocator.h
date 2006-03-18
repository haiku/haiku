/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef CLIENT_MEMORY_ALLOCATOR_H
#define CLIENT_MEMORY_ALLOCATOR_H


#include "MultiLocker.h"

#include <util/DoublyLinkedList.h>


class ServerApp;
struct chunk;
struct block;

struct chunk : DoublyLinkedListLinkImpl<struct chunk> {
	area_id	area;
	uint8*	base;
	size_t	size;
};

struct block : DoublyLinkedListLinkImpl<struct block> {
	struct chunk* chunk;
	uint8*	base;
	size_t	size;
};

typedef DoublyLinkedList<block> block_list;
typedef DoublyLinkedList<chunk> chunk_list;


class ClientMemoryAllocator {
	public:
		ClientMemoryAllocator(ServerApp* application);
		~ClientMemoryAllocator();

		status_t InitCheck();

		void *Allocate(size_t size, void** _address, bool& newArea);
		void Free(void* cookie);

		area_id Area(void* cookie);
		uint32 AreaOffset(void* cookie);

		bool Lock();
		void Unlock();

	private:
		struct block *_AllocateChunk(size_t size, bool& newArea);

		ServerApp*	fApplication;
		MultiLocker	fLock;
		chunk_list	fChunks;
		block_list	fFreeBlocks;
};

#endif	/* CLIENT_MEMORY_ALLOCATOR_H */
