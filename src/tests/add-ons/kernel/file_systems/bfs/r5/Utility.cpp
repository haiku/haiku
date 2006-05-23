/* Utility - some helper classes
 *
 * Copyright 2001-2006, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#include "Utility.h"
#include "Debug.h"

#include <util/kernel_cpp.h>

#include <stdlib.h>
#include <string.h>


bool
sorted_array::_FindInternal(off_t value, int32 &index) const
{
	int32 min = 0, max = CountItems() - 1;
	off_t cmp;
	while (min <= max) {
		index = (min + max) / 2;

		cmp = ValueAt(index) - value;
		if (cmp < 0)
			min = index + 1;
		else if (cmp > 0)
			max = index - 1;
		else
			return true;
	}
	return false;
}


void 
sorted_array::Insert(off_t value)
{
	// if there are more than 8 values in this array, use a
	// binary search, if not, just iterate linearly to find
	// the insertion point
	int32 i;
	if (CountItems() > 8 ) {
		if (!_FindInternal(value, i) && ValueAt(i) <= value)
			i++;
	} else {
		for (i = 0; i < CountItems(); i++) {
			if (ValueAt(i) > value)
				break;
		}
	}

	memmove(&values[i + 1], &values[i], (CountItems() - i) * sizeof(off_t));
	values[i] = HOST_ENDIAN_TO_BFS_INT64(value);
	count = HOST_ENDIAN_TO_BFS_INT64(CountItems() + 1);
}


bool 
sorted_array::Remove(off_t value)
{
	int32 index = Find(value);
	if (index == -1)
		return false;

	memmove(&values[index], &values[index + 1], (CountItems() - index) * sizeof(off_t));
	count = HOST_ENDIAN_TO_BFS_INT64(CountItems() - 1);

	return true;
}


//	#pragma mark -


BlockArray::BlockArray(int32 blockSize)
	:
	fArray(NULL),
	fBlockSize(blockSize),
	fSize(0)
{
}


BlockArray::~BlockArray()
{
	if (fArray)
		free(fArray);
}


int32
BlockArray::Find(off_t value)
{
	if (fArray == NULL)
		return -1;
	
	return fArray->Find(value);
}


status_t
BlockArray::Insert(off_t value)
{
	if (fArray == NULL || fArray->CountItems() + 1 > fMaxBlocks) {
		sorted_array *array = (sorted_array *)realloc(fArray, fSize + fBlockSize);
		if (array == NULL)
			return B_NO_MEMORY;

		if (fArray == NULL)
			array->count = 0;

		fArray = array;
		fSize += fBlockSize;
		fMaxBlocks = fSize / sizeof(off_t) - 1;
	}

	fArray->Insert(value);
	return B_OK;
}


status_t
BlockArray::Remove(off_t value)
{
	if (fArray == NULL)
		return B_ENTRY_NOT_FOUND;

	return fArray->Remove(value) ? (status_t)B_OK : (status_t)B_ENTRY_NOT_FOUND;
}


void 
BlockArray::MakeEmpty()
{
	fArray->count = 0;
}


//	#pragma mark -


extern "C" size_t
strlcpy(char *dest, char const *source, size_t length)
{
	if (length == 0)
		return strlen(source);

	size_t i = 0;
	for (; i < length - 1 && source[i]; i++)
		dest[i] = source[i];

	dest[i] = '\0';

	return i + strlen(source + i);
}

