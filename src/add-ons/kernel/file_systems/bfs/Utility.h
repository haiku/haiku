/*
 * Copyright 2001-2009, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef UTILITY_H
#define UTILITY_H


#include "system_dependencies.h"

#include "bfs.h"


enum inode_type {
	S_DIRECTORY		= S_IFDIR,
	S_FILE			= S_IFREG,
	S_SYMLINK		= S_IFLNK,

	S_INDEX_TYPES	= (S_STR_INDEX | S_INT_INDEX | S_UINT_INDEX
						| S_LONG_LONG_INDEX | S_ULONG_LONG_INDEX
						| S_FLOAT_INDEX | S_DOUBLE_INDEX),

	S_EXTENDED_TYPES = (S_ATTR_DIR | S_ATTR | S_INDEX_DIR)
};


/*!	\a to must be a power of 2.
*/
template<typename IntType, typename RoundType>
inline IntType
round_up(const IntType& value, const RoundType& to)
{
	return (value + (to - 1)) & ~((IntType)to - 1);
}


/*!	\a to must be a power of 2.
*/
template<typename IntType, typename RoundType>
inline IntType
round_down(const IntType& value, const RoundType& to)
{
	return value & ~((IntType)to - 1);
}


inline int32
divide_roundup(int32 num, int32 divisor)
{
	return (num + divisor - 1) / divisor;
}


inline int64
divide_roundup(int64 num, int32 divisor)
{
	return (num + divisor - 1) / divisor;
}


inline int
get_shift(uint64 i)
{
	int c;
	c = 0;
	while (i > 1) {
		i >>= 1;
		c++;
	}
	return c;
}


inline int32
runs_per_block(uint32 blockSize)
{
#ifdef _BOOT_MODE
	using namespace BFS;
#endif

	return blockSize / sizeof(struct block_run);
}


inline int32
double_indirect_max_direct_size(uint32 baseLength, uint32 blockSize)
{
	return baseLength * blockSize;
}


inline int32
double_indirect_max_indirect_size(uint32 baseLength, uint32 blockSize)
{
	return baseLength * double_indirect_max_direct_size(baseLength, blockSize)
		* runs_per_block(blockSize);
}


inline void
get_double_indirect_sizes(uint32 baseLength, uint32 blockSize,
	int32& runsPerBlock, int32& directSize, int32& indirectSize)
{
	runsPerBlock = runs_per_block(blockSize);
	directSize = double_indirect_max_direct_size(baseLength, blockSize);
	indirectSize = baseLength * directSize * runsPerBlock;
}


inline uint32
key_align(uint32 data)
{
	// rounds up to the next off_t boundary
	return (data + sizeof(off_t) - 1) & ~(sizeof(off_t) - 1);
}


inline bool
is_index(int mode)
{
	return (mode & (S_EXTENDED_TYPES | 0777)) == S_INDEX_DIR;
		// That's a stupid check, but the only method to differentiate the
		// index root from an index.
}


inline bool
is_directory(int mode)
{
	return (mode & (S_EXTENDED_TYPES | S_IFDIR)) == S_IFDIR;
}


/*!	Converts the open mode, the open flags given to bfs_open(), into
	access modes, e.g. since O_RDONLY requires read access to the
	file, it will be converted to R_OK.
*/
inline int
open_mode_to_access(int openMode)
{
	openMode &= O_RWMASK;
	if (openMode == O_RDONLY)
		return R_OK;
	if (openMode == O_WRONLY)
		return W_OK;

	return R_OK | W_OK;
}

#endif	// UTILITY_H
