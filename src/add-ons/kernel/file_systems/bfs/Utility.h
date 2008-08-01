/*
 * Copyright 2001-2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef UTILITY_H
#define UTILITY_H


#include "system_dependencies.h"


// Simple array, used for the duplicate handling in the B+Tree
// TODO: this is not endian safe!!!

struct sorted_array {
	public:
		off_t	count;
		off_t	values[0];

		inline int32 Find(off_t value) const;
		void Insert(off_t value);
		bool Remove(off_t value);

	private:
		bool _FindInternal(off_t value, int32 &index) const;
};


inline int32
sorted_array::Find(off_t value) const
{
	int32 i;
	return _FindInternal(value, i) ? i : -1;
}


/*!	\a to must be a power of 2.
*/
template<typename IntType, typename RoundType>
inline IntType
round_up(const IntType& value, const RoundType& to)
{
	return (value + (to - 1)) & ~((IntType)to - 1);
}


inline bool
is_index(int mode)
{
	return (mode & (S_INDEX_DIR | 0777)) == S_INDEX_DIR;
		// That's a stupid check, but AFAIK the only possible method...
}


inline bool
is_directory(int mode)
{
	return (mode & (S_INDEX_DIR | S_ATTR_DIR | S_IFDIR)) == S_IFDIR;
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
