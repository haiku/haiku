/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UNICODE_BLOCKS_H
#define UNICODE_BLOCKS_H


#include <UnicodeBlockObjects.h>


struct unicode_block_entry {
	const char*				name;
	uint32					start;
	uint32					end;
	bool					private_block;
	const unicode_block&	block;

	uint32 Count() const { return end + 1 - start; }
};

extern const unicode_block kNoBlock;
extern const struct unicode_block_entry kUnicodeBlocks[];
extern const uint32 kNumUnicodeBlocks;

#endif	// UNICODE_BLOCKS_H
