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

#endif	/* UTILITY_H */
