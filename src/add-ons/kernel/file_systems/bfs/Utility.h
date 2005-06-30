/* Utility - some helper classes
 *
 * Copyright 2001-2005, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef UTILITY_H
#define UTILITY_H


#include <SupportDefs.h>


// Simple array, used for the duplicate handling in the B+Tree,
// and for the log entries.

struct sorted_array {
	public:
		off_t	count;
		off_t	values[0];

		inline int32 Find(off_t value) const;
		void Insert(off_t value);
		bool Remove(off_t value);

	private:
		bool FindInternal(off_t value,int32 &index) const;
};


inline int32
sorted_array::Find(off_t value) const
{
	int32 i;
	return FindInternal(value,i) ? i : -1;
}


// The BlockArray reserves a multiple of "blockSize" and
// maintain array size for new entries.
// This is used for the in-memory log entries before they
// are written to disk.

class BlockArray {
	public:
		BlockArray(int32 blockSize);
		~BlockArray();

		int32 Find(off_t value);
		status_t Insert(off_t value);
		status_t Remove(off_t value);

		void MakeEmpty();

		int32 CountItems() const { return fArray != NULL ? fArray->count : 0; }
		int32 BlocksUsed() const { return fArray != NULL ? ((fArray->count + 1) * sizeof(off_t) + fBlockSize - 1) / fBlockSize : 0; }
		sorted_array *Array() const { return fArray; }
		int32 Size() const { return fSize; }

	private:
		sorted_array *fArray;
		int32	fBlockSize;
		int32	fSize;
		int32	fMaxBlocks;
};

#endif	/* UTILITY_H */
