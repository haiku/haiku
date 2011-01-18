/*
 * Copyright 2010, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */
#ifndef HTREE_H
#define HTREE_H


#include <AutoDeleter.h>

#include "ext2.h"
#include "DirectoryIterator.h"
#include "HTreeEntryIterator.h"
#include "Inode.h"


#define HTREE_HASH_LEGACY	0
#define HTREE_HASH_HALF_MD4	1
#define HTREE_HASH_TEA		2


struct JournalRevokeHeader;


struct HTreeFakeDirEntry {
	uint32				inode_id;
	uint16				entry_length;
	uint8				name_length;
	uint8				file_type;
	char				file_name[0];
	
	uint32				InodeID() const
		{ return B_LENDIAN_TO_HOST_INT32(inode_id); }


	void				SetEntryLength(uint16 entryLength)
		{ entry_length = B_HOST_TO_LENDIAN_INT16(entryLength); }
} _PACKED;

struct HTreeCountLimit {
	uint16				limit;
	uint16				count;
	
	uint16				Limit() const
		{ return B_LENDIAN_TO_HOST_INT16(limit); }
	uint16				Count() const
		{ return B_LENDIAN_TO_HOST_INT16(count); }
	bool				IsFull() const
		{ return limit == count; }

	void				SetLimit(uint16 value)
		{ limit = B_HOST_TO_LENDIAN_INT16(value); }

	void				SetCount(uint16 value)
		{ count = B_HOST_TO_LENDIAN_INT16(value); }
} _PACKED;

struct HTreeEntry {
	uint32				hash;
	uint32				block;
	
	uint32				Hash() const
		{ return B_LENDIAN_TO_HOST_INT32(hash); }
	uint32				Block() const
		{ return B_LENDIAN_TO_HOST_INT32(block); }

	void				SetHash(uint32 newHash)
		{ hash = B_HOST_TO_LENDIAN_INT32(newHash); }

	void				SetBlock(uint32 newBlock)
		{ block = B_HOST_TO_LENDIAN_INT32(newBlock); }
} _PACKED;

struct HTreeRoot {
	HTreeFakeDirEntry	dot;
	char				dot_entry_name[4];
	HTreeFakeDirEntry	dotdot;
	char				dotdot_entry_name[4];
	
	uint32				reserved;
	uint8				hash_version;
	uint8				root_info_length;
	uint8				indirection_levels;
	uint8				flags;

	HTreeCountLimit		count_limit[0];
	
	bool				IsValid() const;
		// Implemented in HTree.cpp
} _PACKED;

struct HTreeIndirectNode {
	HTreeFakeDirEntry	fake_entry;
	HTreeEntry			entries[0];
} _PACKED;


/*! Hash implementations:
 * References:
 * Main reference: Linux htree patch:
 *    http://thunk.org/tytso/linux/ext3-dxdir/patch-ext3-dxdir-2.5.40
 *  Original MD4 hash: (Modified version used)
 *    http://tools.ietf.org/html/rfc1320
 *  TEA Wikipedia article: (Modified version used)
 *    http://en.wikipedia.org/Tiny_Encryption_Algorithm
 */


class HTree {
public:
								HTree(Volume* volume, Inode* directory);
								~HTree();

			status_t			PrepareForHash();
			uint32				Hash(const char* name, uint8 length);

			status_t			Lookup(const char* name, 
									DirectoryIterator** directory);

	static	status_t			InitDir(Transaction& transaction, Inode* inode,
									Inode* parent);

private:
			status_t			_LookupInNode(uint32 hash, off_t& firstEntry,
									off_t& lastEntry, 
									uint32 remainingIndirects);
			
			uint32				_HashLegacy(const char* name, uint8 length);

	inline	uint32				_MD4F(uint32 x, uint32 y, uint32 z);
	inline	uint32				_MD4G(uint32 x, uint32 y, uint32 z);
	inline	uint32				_MD4H(uint32 x, uint32 y, uint32 z);
	inline	void				_MD4RotateVars(uint32& a, uint32& b, 
									uint32& c, uint32& d);
			void				_HalfMD4Transform(uint32 buffer[4],
									uint32 blocks[8]);
			uint32				_HashHalfMD4(const char* name, uint8 length);
			
			void				_TEATransform(uint32 buffer[4],
									uint32 blocks[4]);
			uint32				_HashTEA(const char* name, uint8 length);
			
			void				_PrepareBlocksForHash(const char* string,
									uint32 length, uint32* blocks, uint32 numBlocks);

	inline	status_t			_FallbackToLinearIteration(
									DirectoryIterator** iterator);

			bool				fIndexed;
			uint32				fBlockSize;
			Inode*				fDirectory;
			uint8				fHashVersion;
			uint32				fHashSeed[4];
			HTreeEntryIterator*	fRootEntry;
			ObjectDeleter<HTreeEntryIterator> fRootEntryDeleter;
};

#endif	// HTREE_H

