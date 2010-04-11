// SuperBlock.h
//
// Copyright (c) 2003, Ingo Weinhold (bonefish@cs.tu-berlin.de)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can alternatively use *this file* under the terms of the the MIT
// license included in this package.

#ifndef SUPER_BLOCK_H
#define SUPER_BLOCK_H

#include "endianess.h"
#include "reiserfs.h"

class SuperBlock {
public:
	SuperBlock();
	~SuperBlock();

	status_t Init(int device, off_t offset = 0);

	uint32 GetFormatVersion() const { return fFormatVersion; }

	uint16 GetBlockSize() const
	{
		return (GetFormatVersion() == REISERFS_3_6
			? le2h(fCurrentData->s_blocksize) : le2h(fOldData->s_blocksize));
	}

	uint32 CountBlocks() const
	{
		return (GetFormatVersion() == REISERFS_3_6
			? le2h(fCurrentData->s_block_count)
			: le2h(fOldData->s_block_count));
	}

	uint32 CountFreeBlocks() const
	{
		return (GetFormatVersion() == REISERFS_3_6
			? le2h(fCurrentData->s_free_blocks)
			: le2h(fOldData->s_free_blocks));
	}

	uint16 GetVersion() const
	{
		return (GetFormatVersion() == REISERFS_3_6
			? le2h(fCurrentData->s_version) : REISERFS_VERSION_0);
	}

	uint32 GetRootBlock() const
	{
		return (GetFormatVersion() == REISERFS_3_6
			? le2h(fCurrentData->s_root_block) : le2h(fOldData->s_root_block));
	}

	uint16 GetState() const
	{
		return (GetFormatVersion() == REISERFS_3_6
			? le2h(fCurrentData->s_state) : le2h(fOldData->s_state));
	}

	uint32 GetHashFunctionCode() const
	{
		return (GetFormatVersion() == REISERFS_3_6
			? le2h(fCurrentData->s_hash_function_code) : UNSET_HASH);
	}

	uint16 GetTreeHeight() const
	{
		return (GetFormatVersion() == REISERFS_3_6
			? le2h(fCurrentData->s_tree_height)
			: le2h(fOldData->s_tree_height));
	}

	status_t GetLabel(char* buffer, size_t bufferSize) const;

private:
	uint32	fFormatVersion;
	union {
		reiserfs_super_block	*fCurrentData;
		reiserfs_super_block_v1	*fOldData;
	};
};

#endif	// SUPER_BLOCK_H
