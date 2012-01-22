/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
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

			void*				Allocate(size_t size, block** _address,
									bool& newArea);
			void				Free(block* cookie);

			void				Dump();

private:
			struct block*		_AllocateChunk(size_t size, bool& newArea);

private:
			ServerApp*			fApplication;
			chunk_list			fChunks;
			block_list			fFreeBlocks;
};


class AreaMemory {
public:
	virtual						~AreaMemory() {}

	virtual area_id				Area() = 0;
	virtual uint8*				Address() = 0;
	virtual uint32				AreaOffset() = 0;
};


class ClientMemory : public AreaMemory {
public:
								ClientMemory();

	virtual						~ClientMemory();

			void*				Allocate(ClientMemoryAllocator* allocator,
									size_t size, bool& newArea);

	virtual area_id				Area();
	virtual uint8*				Address();
	virtual uint32				AreaOffset();
	
private:
			ClientMemoryAllocator*	fAllocator;
			block*				fBlock;
};


/*! Just clones an existing area. */
class ClonedAreaMemory  : public AreaMemory{
public:
								ClonedAreaMemory();
	virtual						~ClonedAreaMemory();

			void*				Clone(area_id area, uint32 offset);

	virtual area_id				Area();
	virtual uint8*				Address();
	virtual uint32				AreaOffset();

private:
			area_id		fClonedArea;
			uint32		fOffset;
			uint8*		fBase;
};


#endif	/* CLIENT_MEMORY_ALLOCATOR_H */
