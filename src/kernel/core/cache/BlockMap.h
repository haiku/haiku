/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef BLOCK_MAP_H
#define BLOCK_MAP_H


#include <OS.h>
#include <util/khash.h>


class BlockMap {
	public:
		BlockMap(off_t size);
		~BlockMap();

		status_t InitCheck() const;

		void SetSize(off_t size);
		off_t Size() const { return fSize; }

		status_t Remove(off_t offset, off_t count = 1);
		status_t Set(off_t offset, addr_t address);
		status_t Get(off_t offset, addr_t &address);

	private:
		struct block_entry;
		status_t GetBlockEntry(off_t offset, block_entry **_entry);

		hash_table	*fHashTable;
		off_t		fSize;
};

#endif	/* BLOCK_MAP_H */
